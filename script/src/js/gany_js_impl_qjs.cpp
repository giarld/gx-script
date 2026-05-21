//
// Created by Gxin on 25-5-23.
//

#include "gany_js_impl_qjs.h"
#include "gany_to_qjs.h"

#include "gx/gglobal.h"
#include "gx/gfile.h"
#include "gx/debug.h"

#include <atomic>
#include <cctype>
#include <csignal>
#include <condition_variable>
#include <cstdio>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>


namespace
{
constexpr const char *GAnyModulePrefix = "gany:";

enum class GxDebuggerStepKind
{
    None,
    Into,
    Over,
    Out,
};

enum class GxDebuggerPauseReason
{
    None,
    Pause,
    Step,
    Breakpoint,
    Exception,
    DebuggerStatement,
};

struct GxDebuggerLocation
{
    bool valid = false;
    std::string file;
    int line = 0;
    int col = 0;
    uint64_t frameId = 0;
    uint32_t frameDepth = 0;
    uint32_t pcOffset = 0;
    JSDebuggerPollKind pollKind = JS_DEBUGGER_POLL_OPCODE;
};

struct GxDebuggerBreakpointView
{
    size_t id = 0;
    std::string file;
    int line = 0;
    std::string condition;
};

struct GxDebuggerBreakpoint
{
    std::string condition;
};

struct GxDebuggerSuppressedBreakpoint
{
    std::string file;
    int line = 0;
    uint32_t pcOffset = 0;
};

enum GxDebuggerFastFlag : uint32_t
{
    GxDebuggerFastFlag_Evaluating = 1u << 0u,
    GxDebuggerFastFlag_PauseRequested = 1u << 1u,
    GxDebuggerFastFlag_StepRequested = 1u << 2u,
    GxDebuggerFastFlag_HasBreakpoints = 1u << 3u,
    GxDebuggerFastFlag_PauseOnException = 1u << 4u,
};

struct GxDebuggerPollSnapshot
{
    bool evaluatingWatches = false;
    bool suppressed = false;
    bool pauseRequested = false;
    bool pauseOnException = false;
    bool printStackOnBreak = true;
    bool interactiveOnBreak = false;
    bool trapOnBreak = true;
    GxDebuggerStepKind stepKind = GxDebuggerStepKind::None;
    GxDebuggerLocation stepOrigin;
};

struct GAnyModuleExports
{
    bool valid = false;
    GAny defaultValue;
    std::vector<std::pair<std::string, GAny>> namedExports;
};

struct GxQjsDebuggerState
{
    mutable std::recursive_mutex syncMutex;
    std::condition_variable_any controlCondition;
    std::map<std::pair<std::string, int>, GxDebuggerBreakpoint> breakpoints;
    bool pauseRequested = false;
    bool resumeRequested = false;
    bool pauseOnException = false;
    bool trapOnBreak = true;
    bool interactiveOnBreak = false;
    bool printStackOnBreak = true;
    bool evaluatingWatches = false;
    std::atomic<uint32_t> fastFlags{0};
    bool paused = false;
    bool livePaused = false;
    GxDebuggerPauseReason pauseReason = GxDebuggerPauseReason::None;
    GxDebuggerLocation pauseLocation;
    GxDebuggerStepKind pauseStepKind = GxDebuggerStepKind::None;
    GxDebuggerLocation pauseStepOrigin;
    GxDebuggerStepKind stepKind = GxDebuggerStepKind::None;
    GxDebuggerLocation stepOrigin;
    std::vector<std::string> watches;
    std::map<uint64_t, GxDebuggerSuppressedBreakpoint> suppressedBreakpoints;
};

static void nativeDebugBreak()
{
#if defined(SIGTRAP)
    std::raise(SIGTRAP);
#else
    std::raise(SIGABRT);
#endif
}

static GxQjsDebuggerState *getDebuggerState(JS_State *jsState)
{
    return jsState ? static_cast<GxQjsDebuggerState *>(jsState->debuggerState) : nullptr;
}

static const GxQjsDebuggerState *getDebuggerState(const JS_State *jsState)
{
    return jsState ? static_cast<const GxQjsDebuggerState *>(jsState->debuggerState) : nullptr;
}

static GxQjsDebuggerState *getDebuggerState(JSContext *ctx)
{
    JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    return getDebuggerState(jsState);
}

static bool isDebuggerOwnerThread(const JS_State *jsState)
{
    return jsState && GThread::currentThreadId() == jsState->threadId;
}

static int gxQjsDebuggerPoll(JSContext *, const char *filename, int lineNum, int colNum,
                             uint64_t frameId, uint32_t frameDepth, uint32_t pcOffset,
                             JSDebuggerPollKind pollKind, JSValueConst eventData,
                             JSDebuggerLocalsGetter *getLocals, void *getLocalsOpaque, void *opaque);
static std::string trimDebuggerCommand(const std::string &command);
static std::vector<GxDebuggerBreakpointView> collectBreakpointViews(const GxQjsDebuggerState *state);
static bool parseBreakpointSpec(JSContext *ctx, const std::string &spec, const std::string *currentFile,
                                std::string *fileOut, int *lineOut);
static bool splitBreakpointCondition(const std::string &spec, std::string *locationOut, std::string *conditionOut);
static bool removeBreakpointBySpec(JSContext *ctx, GxQjsDebuggerState *state, const std::string &spec,
                                   const std::string *currentFile);
static bool tryGetBreakpointAt(const GxQjsDebuggerState *state, const std::string &file, int lineNum,
                               GxDebuggerBreakpoint *breakpointOut);
static void setDebuggerBreakpoint(GxQjsDebuggerState *state, const std::string &file, int line,
                                  const std::string &condition);
static bool eraseDebuggerBreakpoint(GxQjsDebuggerState *state, const std::string &file, int line);
static void clearDebuggerBreakpoint(GxQjsDebuggerState *state, const std::string &file, int line);
static void clearAllDebuggerBreakpoints(GxQjsDebuggerState *state);
static void setDebuggerTrapOnBreak(GxQjsDebuggerState *state, bool enabled);
static void setDebuggerInteractiveOnBreak(GxQjsDebuggerState *state, bool enabled);
static void setDebuggerPrintStackOnBreak(GxQjsDebuggerState *state, bool enabled);
static void setDebuggerPauseOnException(GxQjsDebuggerState *state, bool enabled);
static void addDebuggerWatch(GxQjsDebuggerState *state, const std::string &expression);
static void clearDebuggerWatch(GxQjsDebuggerState *state, const std::string &expression);
static void clearAllDebuggerWatches(GxQjsDebuggerState *state);
static void printBreakpointList(const GxQjsDebuggerState *state);
static void beginWatchEvaluation(JSContext *ctx, GxQjsDebuggerState *state);
static void endWatchEvaluation(JSContext *ctx, GxQjsDebuggerState *state);
static void refreshDebuggerFastFlagsLocked(GxQjsDebuggerState *state);
static uint32_t computeDebuggerPollMask(const GxQjsDebuggerState *state);
static GAny buildDebuggerBreakpointListObject(const GxQjsDebuggerState *state);
static GAny buildDebuggerPauseStateObject(const GxQjsDebuggerState *state);
static bool isDebuggerLivePaused(const GxQjsDebuggerState *state);
static void waitForHostDebuggerCommand(GxQjsDebuggerState *state);

static const char *pauseReasonName(GxDebuggerPauseReason reason)
{
    switch (reason) {
    case GxDebuggerPauseReason::Pause:
        return "pause";
    case GxDebuggerPauseReason::Step:
        return "step";
    case GxDebuggerPauseReason::Breakpoint:
        return "breakpoint";
    case GxDebuggerPauseReason::Exception:
        return "exception";
    case GxDebuggerPauseReason::DebuggerStatement:
        return "debuggerStatement";
    case GxDebuggerPauseReason::None:
        break;
    }
    return nullptr;
}

static const char *stepKindStateName(GxDebuggerStepKind kind)
{
    switch (kind) {
    case GxDebuggerStepKind::Into:
        return "into";
    case GxDebuggerStepKind::Over:
        return "over";
    case GxDebuggerStepKind::Out:
        return "out";
    case GxDebuggerStepKind::None:
        break;
    }
    return nullptr;
}

static void clearPausedState(GxQjsDebuggerState *state)
{
    if (!state) {
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    state->paused = false;
    state->livePaused = false;
    state->pauseReason = GxDebuggerPauseReason::None;
    state->pauseLocation = {};
    state->pauseStepKind = GxDebuggerStepKind::None;
    state->pauseStepOrigin = {};
}

static void deactivateLivePausedState(GxQjsDebuggerState *state)
{
    if (!state) {
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    state->livePaused = false;
}

static void updateDebuggerHandler(JSContext *ctx, GxQjsDebuggerState *state)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    const uint32_t pollMask = computeDebuggerPollMask(state);
    JS_SetDebuggerHandlerWithPollMask(rt, pollMask ? gxQjsDebuggerPoll : nullptr,
                                      pollMask ? state : nullptr, pollMask);
    JS_SetDebuggerStatementHandler(rt, state ? gxQjsDebuggerPoll : nullptr, state);
}

static JSValue js_debugBreak(JSContext *, JSValueConst, int, JSValueConst *)
{
    nativeDebugBreak();
    return JS_UNDEFINED;
}

static JSValue js_gxDebuggerIsPaused(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    GxQjsDebuggerState *state = getDebuggerState(ctx);
    if (!state) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }
    return JS_NewBool(ctx, isDebuggerLivePaused(state));
}

static JSValue js_gxDebuggerGetPauseState(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    GxQjsDebuggerState *state = getDebuggerState(ctx);
    JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    if (!state || !jsState) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }
    return GAnyToQJS::makeGAnyToJsValue(jsState, buildDebuggerPauseStateObject(state), true);
}

static void flushDebuggerOutput()
{
    std::fflush(stdout);
    std::fflush(stderr);
}

static bool getCurrentSourceFile(JSContext *ctx, std::string *fileOut)
{
    if (!ctx || !fileOut) {
        return false;
    }

    for (int stackLevel = 0; stackLevel < 32; ++stackLevel) {
        JSAtom atom = JS_GetScriptOrModuleName(ctx, stackLevel);
        if (atom == JS_ATOM_NULL) {
            continue;
        }

        const char *file = JS_AtomToCString(ctx, atom);
        JS_FreeAtom(ctx, atom);
        if (!file) {
            JS_FreeValue(ctx, JS_GetException(ctx));
            continue;
        }

        *fileOut = file;
        JS_FreeCString(ctx, file);
        if (!fileOut->empty()) {
            return true;
        }
    }
    return false;
}

static bool resolveBreakpointFileToken(JSContext *ctx, const std::string &fileToken, const std::string *currentFile,
                                       std::string *fileOut)
{
    if (!fileOut) {
        return false;
    }

    const std::string trimmed = trimDebuggerCommand(fileToken);
    if (trimmed.empty()) {
        return false;
    }
    if (trimmed != ".") {
        *fileOut = trimmed;
        return true;
    }
    if (currentFile && !currentFile->empty()) {
        *fileOut = *currentFile;
        return true;
    }
    return getCurrentSourceFile(ctx, fileOut);
}

static bool readBreakpointConditionOption(JSContext *ctx, int argc, JSValueConst *argv, std::string *conditionOut)
{
    if (!conditionOut) {
        return false;
    }
    conditionOut->clear();
    if (argc < 3 || JS_IsUndefined(argv[2]) || JS_IsNull(argv[2])) {
        return true;
    }

    JSValue conditionVal = JS_GetPropertyStr(ctx, argv[2], "condition");
    if (JS_IsException(conditionVal)) {
        return false;
    }
    if (JS_IsUndefined(conditionVal) || JS_IsNull(conditionVal)) {
        JS_FreeValue(ctx, conditionVal);
        return true;
    }

    const char *condition = JS_ToCString(ctx, conditionVal);
    JS_FreeValue(ctx, conditionVal);
    if (!condition) {
        return false;
    }

    *conditionOut = trimDebuggerCommand(condition);
    JS_FreeCString(ctx, condition);
    return true;
}

static JSValue js_gxDebuggerSetBreakpoint(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "setBreakpoint(file, line[, options]) expects at least 2 arguments");
    }

    GxQjsDebuggerState *state = getDebuggerState(ctx);
    if (!state) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }

    const char *file = JS_ToCString(ctx, argv[0]);
    if (!file) {
        return JS_EXCEPTION;
    }

    std::string resolvedFile;
    if (!resolveBreakpointFileToken(ctx, file, nullptr, &resolvedFile)) {
        JS_FreeCString(ctx, file);
        return JS_ThrowReferenceError(ctx, "could not resolve current source file for breakpoint");
    }

    int32_t line = 0;
    if (JS_ToInt32(ctx, &line, argv[1]) < 0) {
        JS_FreeCString(ctx, file);
        return JS_EXCEPTION;
    }
    if (line <= 0) {
        JS_FreeCString(ctx, file);
        return JS_ThrowRangeError(ctx, "breakpoint line must be positive");
    }

    std::string condition;
    if (!readBreakpointConditionOption(ctx, argc, argv, &condition)) {
        JS_FreeCString(ctx, file);
        return JS_EXCEPTION;
    }

    setDebuggerBreakpoint(state, resolvedFile, line, condition);
    updateDebuggerHandler(ctx, state);
    JS_FreeCString(ctx, file);
    return JS_UNDEFINED;
}

static JSValue js_gxDebuggerClearBreakpoint(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "clearBreakpoint(file, line) expects 2 arguments");
    }

    GxQjsDebuggerState *state = getDebuggerState(ctx);
    if (!state) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }

    const char *file = JS_ToCString(ctx, argv[0]);
    if (!file) {
        return JS_EXCEPTION;
    }

    std::string resolvedFile;
    if (!resolveBreakpointFileToken(ctx, file, nullptr, &resolvedFile)) {
        JS_FreeCString(ctx, file);
        return JS_ThrowReferenceError(ctx, "could not resolve current source file for breakpoint");
    }

    int32_t line = 0;
    if (JS_ToInt32(ctx, &line, argv[1]) < 0) {
        JS_FreeCString(ctx, file);
        return JS_EXCEPTION;
    }

    clearDebuggerBreakpoint(state, resolvedFile, line);
    updateDebuggerHandler(ctx, state);
    JS_FreeCString(ctx, file);
    return JS_UNDEFINED;
}

static JSValue js_gxDebuggerClearAllBreakpoints(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    GxQjsDebuggerState *state = getDebuggerState(ctx);
    if (!state) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }

    clearAllDebuggerBreakpoints(state);
    updateDebuggerHandler(ctx, state);
    return JS_UNDEFINED;
}

static JSValue js_gxDebuggerListBreakpoints(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    GxQjsDebuggerState *state = getDebuggerState(ctx);
    if (!state) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }

    const auto breakpoints = collectBreakpointViews(state);
    JSValue list = JS_NewArray(ctx);
    if (JS_IsException(list)) {
        return JS_EXCEPTION;
    }

    for (uint32_t i = 0; i < breakpoints.size(); ++i) {
        const auto &breakpoint = breakpoints[i];
        JSValue item = JS_NewObject(ctx);
        if (JS_IsException(item)) {
            JS_FreeValue(ctx, list);
            return JS_EXCEPTION;
        }

        if (JS_SetPropertyStr(ctx, item, "id", JS_NewInt32(ctx, static_cast<int32_t>(breakpoint.id))) < 0
            || JS_SetPropertyStr(ctx, item, "file", JS_NewString(ctx, breakpoint.file.c_str())) < 0
            || JS_SetPropertyStr(ctx, item, "line", JS_NewInt32(ctx, breakpoint.line)) < 0
            || JS_SetPropertyStr(ctx, item, "condition", JS_NewString(ctx, breakpoint.condition.c_str())) < 0
            || JS_SetPropertyUint32(ctx, list, i, item) < 0) {
            JS_FreeValue(ctx, item);
            JS_FreeValue(ctx, list);
            return JS_EXCEPTION;
        }
    }

    return list;
}

static void requestDebuggerPause(GxQjsDebuggerState *state)
{
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    clearPausedState(state);
    state->pauseRequested = true;
    state->resumeRequested = false;
    state->stepKind = GxDebuggerStepKind::None;
    state->stepOrigin = {};
    refreshDebuggerFastFlagsLocked(state);
    state->controlCondition.notify_all();
}

static JSValue js_gxDebuggerPause(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    GxQjsDebuggerState *state = getDebuggerState(ctx);
    if (!state) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }

    requestDebuggerPause(state);
    updateDebuggerHandler(ctx, state);
    return JS_UNDEFINED;
}

static void requestDebuggerResume(GxQjsDebuggerState *state)
{
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    const bool wasLivePaused = state->livePaused;
    clearPausedState(state);
    state->pauseRequested = false;
    state->resumeRequested = wasLivePaused;
    state->stepKind = GxDebuggerStepKind::None;
    state->stepOrigin = {};
    refreshDebuggerFastFlagsLocked(state);
    state->controlCondition.notify_all();
}

static JSValue js_gxDebuggerResume(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    GxQjsDebuggerState *state = getDebuggerState(ctx);
    if (!state) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }

    requestDebuggerResume(state);
    updateDebuggerHandler(ctx, state);
    return JS_UNDEFINED;
}

static void requestDebuggerStep(GxQjsDebuggerState *state, GxDebuggerStepKind kind)
{
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    GxDebuggerLocation origin;
    const bool wasLivePaused = state && state->paused && state->livePaused;
    if (wasLivePaused) {
        origin = state->pauseLocation;
    }
    clearPausedState(state);
    state->pauseRequested = false;
    state->resumeRequested = wasLivePaused;
    state->stepKind = kind;
    state->stepOrigin = origin;
    refreshDebuggerFastFlagsLocked(state);
    state->controlCondition.notify_all();
}

static JSValue js_gxDebuggerStepInto(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    GxQjsDebuggerState *state = getDebuggerState(ctx);
    if (!state) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }

    requestDebuggerStep(state, GxDebuggerStepKind::Into);
    updateDebuggerHandler(ctx, state);
    return JS_UNDEFINED;
}

static JSValue js_gxDebuggerStepOver(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    GxQjsDebuggerState *state = getDebuggerState(ctx);
    if (!state) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }

    requestDebuggerStep(state, GxDebuggerStepKind::Over);
    updateDebuggerHandler(ctx, state);
    return JS_UNDEFINED;
}

static JSValue js_gxDebuggerStepOut(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    GxQjsDebuggerState *state = getDebuggerState(ctx);
    if (!state) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }

    requestDebuggerStep(state, GxDebuggerStepKind::Out);
    updateDebuggerHandler(ctx, state);
    return JS_UNDEFINED;
}

static JSValue js_gxDebuggerSetTrapOnBreak(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    GxQjsDebuggerState *state = getDebuggerState(ctx);
    if (!state) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }

    if (argc == 0) {
        setDebuggerTrapOnBreak(state, true);
        return JS_UNDEFINED;
    }

    const int enabled = JS_ToBool(ctx, argv[0]);
    if (enabled < 0) {
        return JS_EXCEPTION;
    }

    setDebuggerTrapOnBreak(state, enabled == 1);
    return JS_UNDEFINED;
}

static JSValue js_gxDebuggerSetInteractiveOnBreak(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    GxQjsDebuggerState *state = getDebuggerState(ctx);
    if (!state) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }

    if (argc == 0) {
        setDebuggerInteractiveOnBreak(state, true);
        return JS_UNDEFINED;
    }

    const int enabled = JS_ToBool(ctx, argv[0]);
    if (enabled < 0) {
        return JS_EXCEPTION;
    }

    setDebuggerInteractiveOnBreak(state, enabled == 1);
    return JS_UNDEFINED;
}

static JSValue js_gxDebuggerSetPrintStackOnBreak(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    GxQjsDebuggerState *state = getDebuggerState(ctx);
    if (!state) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }

    if (argc == 0) {
        setDebuggerPrintStackOnBreak(state, true);
        return JS_UNDEFINED;
    }

    const int enabled = JS_ToBool(ctx, argv[0]);
    if (enabled < 0) {
        return JS_EXCEPTION;
    }

    setDebuggerPrintStackOnBreak(state, enabled == 1);
    return JS_UNDEFINED;
}

static JSValue js_gxDebuggerSetPauseOnException(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    GxQjsDebuggerState *state = getDebuggerState(ctx);
    if (!state) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }

    if (argc == 0) {
        setDebuggerPauseOnException(state, true);
        updateDebuggerHandler(ctx, state);
        return JS_UNDEFINED;
    }

    const int enabled = JS_ToBool(ctx, argv[0]);
    if (enabled < 0) {
        return JS_EXCEPTION;
    }

    setDebuggerPauseOnException(state, enabled == 1);
    updateDebuggerHandler(ctx, state);
    return JS_UNDEFINED;
}

static JSValue js_gxDebuggerWatch(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "watch(expression) expects 1 argument");
    }

    GxQjsDebuggerState *state = getDebuggerState(ctx);
    if (!state) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }

    const char *expr = JS_ToCString(ctx, argv[0]);
    if (!expr) {
        return JS_EXCEPTION;
    }

    addDebuggerWatch(state, expr);
    JS_FreeCString(ctx, expr);
    return JS_UNDEFINED;
}

static JSValue js_gxDebuggerClearWatch(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "clearWatch(expression) expects 1 argument");
    }

    GxQjsDebuggerState *state = getDebuggerState(ctx);
    if (!state) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }

    const char *expr = JS_ToCString(ctx, argv[0]);
    if (!expr) {
        return JS_EXCEPTION;
    }

    const std::string target = expr;
    JS_FreeCString(ctx, expr);
    clearDebuggerWatch(state, target);
    return JS_UNDEFINED;
}

static JSValue js_gxDebuggerClearAllWatches(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    GxQjsDebuggerState *state = getDebuggerState(ctx);
    if (!state) {
        return JS_ThrowInternalError(ctx, "debugger state is not initialized");
    }

    clearAllDebuggerWatches(state);
    return JS_UNDEFINED;
}

static bool isBreakpointFileMatch(const std::string &breakpointFile, const std::string &runtimeFile)
{
    if (breakpointFile == runtimeFile) {
        return true;
    }
    if (runtimeFile.size() > breakpointFile.size()
        && runtimeFile.compare(runtimeFile.size() - breakpointFile.size(), breakpointFile.size(), breakpointFile) == 0
        && runtimeFile[runtimeFile.size() - breakpointFile.size() - 1] == '/') {
        return true;
    }
    return false;
}

static bool tryGetBreakpointAt(const GxQjsDebuggerState *state, const std::string &file, int lineNum,
                               GxDebuggerBreakpoint *breakpointOut)
{
    if (!state || !breakpointOut) {
        return false;
    }
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    for (const auto &breakpoint: state->breakpoints) {
        if (breakpoint.first.second == lineNum && isBreakpointFileMatch(breakpoint.first.first, file)) {
            *breakpointOut = breakpoint.second;
            return true;
        }
    }
    return false;
}

static std::vector<GxDebuggerBreakpointView> collectBreakpointViews(const GxQjsDebuggerState *state)
{
    std::vector<GxDebuggerBreakpointView> breakpoints;
    if (!state) {
        return breakpoints;
    }

    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    breakpoints.reserve(state->breakpoints.size());
    size_t id = 1;
    for (const auto &breakpoint: state->breakpoints) {
        breakpoints.push_back({id++, breakpoint.first.first, breakpoint.first.second, breakpoint.second.condition});
    }
    return breakpoints;
}

static GAny buildDebuggerBreakpointListObject(const GxQjsDebuggerState *state)
{
    GAny list = GAny::array();
    for (const auto &breakpoint: collectBreakpointViews(state)) {
        GAny item = GAny::object();
        item["id"] = static_cast<int64_t>(breakpoint.id);
        item["file"] = breakpoint.file;
        item["line"] = breakpoint.line;
        item["condition"] = breakpoint.condition;
        list.pushBack(item);
    }
    return list;
}

static bool resolveHostBreakpointFileToken(JSContext *ctx, const GxQjsDebuggerState *state,
                                           const std::string &fileToken, std::string *fileOut)
{
    std::string currentFile;
    const std::string *currentFilePtr = nullptr;
    if (state) {
        std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
        // Only treat the last pause location as "." while execution is still live-paused.
        // After a non-interactive break resumes, pauseLocation remains as a snapshot and
        // must not shadow the current executing source file.
        if (state->livePaused && state->pauseLocation.valid && !state->pauseLocation.file.empty()) {
            currentFile = state->pauseLocation.file;
            currentFilePtr = &currentFile;
        }
    }
    return resolveBreakpointFileToken(ctx, fileToken, currentFilePtr, fileOut);
}

static void setDebuggerBreakpoint(GxQjsDebuggerState *state, const std::string &file, int line,
                                  const std::string &condition)
{
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    state->breakpoints[{file, line}] = GxDebuggerBreakpoint{condition};
    state->suppressedBreakpoints.clear();
    refreshDebuggerFastFlagsLocked(state);
}

static bool eraseDebuggerBreakpoint(GxQjsDebuggerState *state, const std::string &file, int line)
{
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    const bool erased = state->breakpoints.erase({file, line}) > 0;
    if (erased) {
        state->suppressedBreakpoints.clear();
        refreshDebuggerFastFlagsLocked(state);
    }
    return erased;
}

static void clearDebuggerBreakpoint(GxQjsDebuggerState *state, const std::string &file, int line)
{
    eraseDebuggerBreakpoint(state, file, line);
}

static void clearAllDebuggerBreakpoints(GxQjsDebuggerState *state)
{
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    state->breakpoints.clear();
    state->suppressedBreakpoints.clear();
    refreshDebuggerFastFlagsLocked(state);
}

static void setDebuggerTrapOnBreak(GxQjsDebuggerState *state, bool enabled)
{
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    state->trapOnBreak = enabled;
}

static void setDebuggerInteractiveOnBreak(GxQjsDebuggerState *state, bool enabled)
{
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    state->interactiveOnBreak = enabled;
}

static void setDebuggerPrintStackOnBreak(GxQjsDebuggerState *state, bool enabled)
{
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    state->printStackOnBreak = enabled;
}

static void setDebuggerPauseOnException(GxQjsDebuggerState *state, bool enabled)
{
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    state->pauseOnException = enabled;
    refreshDebuggerFastFlagsLocked(state);
}

static void addDebuggerWatch(GxQjsDebuggerState *state, const std::string &expression)
{
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    state->watches.emplace_back(expression);
}

static void clearDebuggerWatch(GxQjsDebuggerState *state, const std::string &expression)
{
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    for (auto it = state->watches.begin(); it != state->watches.end(); ++it) {
        if (*it == expression) {
            state->watches.erase(it);
            break;
        }
    }
}

static void clearAllDebuggerWatches(GxQjsDebuggerState *state)
{
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    state->watches.clear();
}

static bool parseBreakpointLine(const std::string &text, int *lineOut)
{
    if (!lineOut || text.empty()) {
        return false;
    }

    int line = 0;
    for (char ch: text) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            return false;
        }
        line = line * 10 + (ch - '0');
    }
    if (line <= 0) {
        return false;
    }

    *lineOut = line;
    return true;
}

static bool parseBreakpointSpec(JSContext *ctx, const std::string &spec, const std::string *currentFile,
                                std::string *fileOut, int *lineOut)
{
    if (!fileOut || !lineOut) {
        return false;
    }

    const std::string trimmed = trimDebuggerCommand(spec);
    if (trimmed.empty()) {
        return false;
    }

    const size_t colon = trimmed.rfind(':');
    if (colon == std::string::npos || colon == 0 || colon + 1 >= trimmed.size()) {
        return false;
    }

    int line = 0;
    if (!parseBreakpointLine(trimmed.substr(colon + 1), &line)) {
        return false;
    }

    if (!resolveBreakpointFileToken(ctx, trimmed.substr(0, colon), currentFile, fileOut)) {
        return false;
    }
    *lineOut = line;
    return true;
}

static bool splitBreakpointCondition(const std::string &spec, std::string *locationOut, std::string *conditionOut)
{
    if (!locationOut || !conditionOut) {
        return false;
    }

    const std::string trimmed = trimDebuggerCommand(spec);
    const std::string marker = " if ";
    const size_t markerPos = trimmed.find(marker);
    if (markerPos == std::string::npos) {
        *locationOut = trimmed;
        conditionOut->clear();
        return true;
    }

    *locationOut = trimDebuggerCommand(trimmed.substr(0, markerPos));
    *conditionOut = trimDebuggerCommand(trimmed.substr(markerPos + marker.size()));
    return !locationOut->empty() && !conditionOut->empty();
}

static bool removeBreakpointBySpec(JSContext *ctx, GxQjsDebuggerState *state, const std::string &spec,
                                   const std::string *currentFile)
{
    if (!state) {
        return false;
    }

    int id = 0;
    if (parseBreakpointLine(spec, &id)) {
        const auto breakpoints = collectBreakpointViews(state);
        if (id >= 1 && static_cast<size_t>(id) <= breakpoints.size()) {
            const auto &breakpoint = breakpoints[static_cast<size_t>(id) - 1];
            return eraseDebuggerBreakpoint(state, breakpoint.file, breakpoint.line);
        }
    }

    std::string file;
    int line = 0;
    if (!parseBreakpointSpec(ctx, spec, currentFile, &file, &line)) {
        return false;
    }
    return eraseDebuggerBreakpoint(state, file, line);
}

static void printBreakpointList(const GxQjsDebuggerState *state)
{
    const auto breakpoints = collectBreakpointViews(state);
    if (breakpoints.empty()) {
        std::fprintf(stderr, "[GxDebugger] no breakpoints\n");
        return;
    }

    std::fprintf(stderr, "[GxDebugger] breakpoints:\n");
    for (const auto &breakpoint: breakpoints) {
        std::fprintf(stderr, "  #%zu %s:%d", breakpoint.id, breakpoint.file.c_str(), breakpoint.line);
        if (!breakpoint.condition.empty()) {
            std::fprintf(stderr, " if %s", breakpoint.condition.c_str());
        }
        std::fprintf(stderr, "\n");
    }
}

static void refreshDebuggerFastFlagsLocked(GxQjsDebuggerState *state)
{
    uint32_t flags = 0;
    if (state->evaluatingWatches) {
        flags |= GxDebuggerFastFlag_Evaluating;
    }
    if (state->pauseRequested) {
        flags |= GxDebuggerFastFlag_PauseRequested;
    }
    if (state->stepKind != GxDebuggerStepKind::None) {
        flags |= GxDebuggerFastFlag_StepRequested;
    }
    if (!state->breakpoints.empty()) {
        flags |= GxDebuggerFastFlag_HasBreakpoints;
    }
    if (state->pauseOnException) {
        flags |= GxDebuggerFastFlag_PauseOnException;
    }
    state->fastFlags.store(flags, std::memory_order_release);
}

static uint32_t computeDebuggerPollMask(const GxQjsDebuggerState *state)
{
    if (!state) {
        return JS_DEBUGGER_POLL_MASK_NONE;
    }

    uint32_t pollMask = JS_DEBUGGER_POLL_MASK_OPCODE | JS_DEBUGGER_POLL_MASK_CALL;
    if (state->fastFlags.load(std::memory_order_acquire) & GxDebuggerFastFlag_PauseOnException) {
        pollMask |= JS_DEBUGGER_POLL_MASK_EXCEPTION;
    }
    return pollMask;
}

static std::string rewriteTopStackFrameLocation(const std::string &stack, const GxDebuggerLocation *current)
{
    if (!current || !current->valid || stack.empty()) {
        return stack;
    }

    const size_t lineEnd = stack.find('\n');
    const size_t openParen = stack.find(" (");
    if (openParen == std::string::npos || (lineEnd != std::string::npos && openParen > lineEnd)) {
        return stack;
    }

    const size_t closeParen = stack.find(')', openParen + 2);
    if (closeParen == std::string::npos || (lineEnd != std::string::npos && closeParen > lineEnd)) {
        return stack;
    }

    std::string rewritten = stack;
    const std::string location = " (" + current->file + ":" + std::to_string(current->line) + ":"
                                 + std::to_string(current->col) + ")";
    rewritten.replace(openParen, closeParen - openParen + 1, location);
    return rewritten;
}

static void printCurrentJsStack(JSContext *ctx, GxQjsDebuggerState *state,
                                const GxDebuggerLocation *current = nullptr)
{
    JSValue savedBackTrace = JS_UNDEFINED;
    JSValue transientBackTrace = JS_UNDEFINED;
    js_std_cmd(/*ErrorBackTrace*/2, ctx, &savedBackTrace);

    beginWatchEvaluation(ctx, state);
    JSValue errorVal = JS_NewError(ctx);
    if (JS_IsException(errorVal)) {
        JS_FreeValue(ctx, JS_GetException(ctx));
        endWatchEvaluation(ctx, state);
        js_std_cmd(/*SetErrorBackTrace*/4, ctx, &savedBackTrace);
        return;
    }

    JSValue stackVal = JS_GetPropertyStr(ctx, errorVal, "stack");
    if (JS_IsException(stackVal)) {
        JS_FreeValue(ctx, JS_GetException(ctx));
        JS_FreeValue(ctx, errorVal);
        endWatchEvaluation(ctx, state);
        js_std_cmd(/*ErrorBackTrace*/2, ctx, &transientBackTrace);
        JS_FreeValue(ctx, transientBackTrace);
        js_std_cmd(/*SetErrorBackTrace*/4, ctx, &savedBackTrace);
        return;
    }

    const char *stack = JS_ToCString(ctx, stackVal);
    if (stack) {
        const std::string stackText = rewriteTopStackFrameLocation(stack, current);
        std::fprintf(stderr, "%s\n", stackText.c_str());
        JS_FreeCString(ctx, stack);
    }

    JS_FreeValue(ctx, stackVal);
    JS_FreeValue(ctx, errorVal);
    endWatchEvaluation(ctx, state);
    js_std_cmd(/*ErrorBackTrace*/2, ctx, &transientBackTrace);
    JS_FreeValue(ctx, transientBackTrace);
    js_std_cmd(/*SetErrorBackTrace*/4, ctx, &savedBackTrace);
}

static void printWatchException(JSContext *ctx, const std::string &expr)
{
    const JSValue exceptionVal = JS_GetException(ctx);
    const char *exceptionStr = JS_ToCString(ctx, exceptionVal);
    std::fprintf(stderr, "[GxDebugger] watch %s = <exception", expr.c_str());
    if (exceptionStr) {
        std::fprintf(stderr, ": %s", exceptionStr);
        JS_FreeCString(ctx, exceptionStr);
    }
    std::fprintf(stderr, ">\n");
    JS_FreeValue(ctx, exceptionVal);
}

static void printBreakpointConditionException(JSContext *ctx, const std::string &condition)
{
    const JSValue exceptionVal = JS_GetException(ctx);
    const char *exceptionStr = JS_ToCString(ctx, exceptionVal);
    std::fprintf(stderr, "[GxDebugger] breakpoint condition %s = <exception", condition.c_str());
    if (exceptionStr) {
        std::fprintf(stderr, ": %s", exceptionStr);
        JS_FreeCString(ctx, exceptionStr);
    }
    std::fprintf(stderr, ">\n");
    JS_FreeValue(ctx, exceptionVal);
}

static void beginWatchEvaluation(JSContext *ctx, GxQjsDebuggerState *state)
{
    {
        std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
        state->evaluatingWatches = true;
        refreshDebuggerFastFlagsLocked(state);
    }
    JS_SetDebuggerHandler(JS_GetRuntime(ctx), nullptr, nullptr);
    JS_SetDebuggerStatementHandler(JS_GetRuntime(ctx), nullptr, nullptr);
}

static void endWatchEvaluation(JSContext *ctx, GxQjsDebuggerState *state)
{
    {
        std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
        state->evaluatingWatches = false;
        refreshDebuggerFastFlagsLocked(state);
    }
    updateDebuggerHandler(ctx, state);
}

static JSValue evalExpressionWithLocals(JSContext *ctx, GxQjsDebuggerState *state, JSValueConst locals,
                                        const std::string &expr, const char *filename)
{
    const std::string source = "(function(__gx_locals){ with (__gx_locals) { return (" + expr + "); } })";

    beginWatchEvaluation(ctx, state);
    JSValue funcVal = JS_Eval(ctx, source.c_str(), source.size(), filename, JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(funcVal)) {
        endWatchEvaluation(ctx, state);
        return JS_EXCEPTION;
    }

    JSValue argv[] = { JS_DupValue(ctx, locals) };
    JSValue resultVal = JS_Call(ctx, funcVal, JS_UNDEFINED, 1, argv);
    JS_FreeValue(ctx, argv[0]);
    JS_FreeValue(ctx, funcVal);
    endWatchEvaluation(ctx, state);
    return resultVal;
}

static void printWatchValue(JSContext *ctx, GxQjsDebuggerState *state, JSValueConst locals, const std::string &expr)
{
    JSValue resultVal = evalExpressionWithLocals(ctx, state, locals, expr, "<gx-watch>");
    if (JS_IsException(resultVal)) {
        printWatchException(ctx, expr);
        return;
    }

    const char *resultStr = JS_ToCString(ctx, resultVal);
    if (resultStr) {
        std::fprintf(stderr, "[GxDebugger] watch %s = %s\n", expr.c_str(), resultStr);
        JS_FreeCString(ctx, resultStr);
    } else {
        JS_FreeValue(ctx, JS_GetException(ctx));
        std::fprintf(stderr, "[GxDebugger] watch %s = <unprintable>\n", expr.c_str());
    }
    JS_FreeValue(ctx, resultVal);
}

static bool shouldPauseForBreakpointCondition(JSContext *ctx, GxQjsDebuggerState *state,
                                              const GxDebuggerBreakpoint *breakpoint,
                                              JSDebuggerLocalsGetter *getLocals, void *getLocalsOpaque)
{
    if (!breakpoint || breakpoint->condition.empty()) {
        return true;
    }

    JSValue locals = getLocals(ctx, getLocalsOpaque);
    if (JS_IsException(locals)) {
        JS_FreeValue(ctx, JS_GetException(ctx));
        std::fprintf(stderr, "[GxDebugger] failed to collect locals for breakpoint condition\n");
        return true;
    }

    JSValue resultVal = evalExpressionWithLocals(ctx, state, locals, breakpoint->condition, "<gx-breakpoint-condition>");
    JS_FreeValue(ctx, locals);
    if (JS_IsException(resultVal)) {
        printBreakpointConditionException(ctx, breakpoint->condition);
        return true;
    }

    const int result = JS_ToBool(ctx, resultVal);
    JS_FreeValue(ctx, resultVal);
    if (result < 0) {
        JS_FreeValue(ctx, JS_GetException(ctx));
        std::fprintf(stderr, "[GxDebugger] failed to convert breakpoint condition to boolean: %s\n",
                     breakpoint->condition.c_str());
        return true;
    }
    return result == 1;
}

static void printWatches(JSContext *ctx, GxQjsDebuggerState *state, JSValueConst locals)
{
    std::vector<std::string> watches;
    {
        std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
        watches = state->watches;
    }
    for (const std::string &expr: watches) {
        printWatchValue(ctx, state, locals, expr);
    }
}

static std::string trimDebuggerCommand(const std::string &command)
{
    const size_t begin = command.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const size_t end = command.find_last_not_of(" \t\r\n");
    return command.substr(begin, end - begin + 1);
}

static std::string escapeDebuggerSingleLine(const char *text)
{
    if (!text) {
        return {};
    }

    std::string escaped;
    for (const char *p = text; *p; ++p) {
        switch (*p) {
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped += *p;
            break;
        }
    }
    return escaped;
}

static const char *stepKindName(GxDebuggerStepKind kind)
{
    switch (kind) {
    case GxDebuggerStepKind::Into:
        return "step";
    case GxDebuggerStepKind::Over:
        return "next";
    case GxDebuggerStepKind::Out:
        return "finish";
    case GxDebuggerStepKind::None:
        break;
    }
    return "none";
}

static GAny buildDebuggerLocationObject(const GxDebuggerLocation &location)
{
    if (!location.valid) {
        return GAny::null();
    }

    GAny obj = GAny::object();
    obj["file"] = location.file;
    obj["line"] = location.line;
    obj["col"] = location.col;
    obj["frameId"] = static_cast<int64_t>(location.frameId);
    obj["frameDepth"] = static_cast<int64_t>(location.frameDepth);
    obj["pcOffset"] = static_cast<int64_t>(location.pcOffset);
    return obj;
}

static GAny buildDebuggerPauseStateObject(const GxQjsDebuggerState *state)
{
    GAny obj = GAny::object();
    GAny step = GAny::object();
    if (!state) {
        obj["paused"] = false;
        obj["reason"] = GAny::null();
        obj["location"] = GAny::null();
        obj["pendingPause"] = false;
        step["pending"] = false;
        step["kind"] = GAny::null();
        step["origin"] = GAny::null();
        obj["step"] = step;
        obj["breakpointsCount"] = static_cast<int64_t>(0);
        obj["watchesCount"] = static_cast<int64_t>(0);
        return obj;
    }
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    const bool hasPendingStep = state && state->stepKind != GxDebuggerStepKind::None;
    const bool hasPausedStep = state && state->paused && state->pauseReason == GxDebuggerPauseReason::Step
                               && state->pauseStepKind != GxDebuggerStepKind::None;

    obj["paused"] = state && state->livePaused;
    obj["reason"] = (state && state->paused && pauseReasonName(state->pauseReason))
                    ? GAny(std::string(pauseReasonName(state->pauseReason)))
                    : GAny::null();
    obj["location"] = (state && state->paused)
                      ? buildDebuggerLocationObject(state->pauseLocation)
                      : GAny::null();
    obj["pendingPause"] = state && state->pauseRequested;

    step["pending"] = hasPendingStep;
    step["kind"] = (hasPendingStep && stepKindStateName(state->stepKind))
                   ? GAny(std::string(stepKindStateName(state->stepKind)))
                   : (hasPausedStep && stepKindStateName(state->pauseStepKind))
                     ? GAny(std::string(stepKindStateName(state->pauseStepKind)))
                   : GAny::null();
    step["origin"] = hasPendingStep
                     ? buildDebuggerLocationObject(state->stepOrigin)
                     : hasPausedStep
                       ? buildDebuggerLocationObject(state->pauseStepOrigin)
                     : GAny::null();

    obj["step"] = step;
    obj["breakpointsCount"] = static_cast<int64_t>(state ? state->breakpoints.size() : 0);
    obj["watchesCount"] = static_cast<int64_t>(state ? state->watches.size() : 0);
    return obj;
}

static bool isDebuggerLivePaused(const GxQjsDebuggerState *state)
{
    if (!state) {
        return false;
    }
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    return state->livePaused;
}

static GxDebuggerLocation makeDebuggerLocation(const std::string &file, int lineNum, int colNum,
                                               uint64_t frameId, uint32_t frameDepth, uint32_t pcOffset,
                                               JSDebuggerPollKind pollKind)
{
    GxDebuggerLocation location;
    location.valid = true;
    location.file = file;
    location.line = lineNum;
    location.col = colNum;
    location.frameId = frameId;
    location.frameDepth = frameDepth;
    location.pcOffset = pcOffset;
    location.pollKind = pollKind;
    return location;
}

static bool sameDebuggerSourceLocation(const GxDebuggerLocation &lhs, const GxDebuggerLocation &rhs)
{
    return lhs.valid && rhs.valid && lhs.frameId == rhs.frameId && lhs.file == rhs.file
           && lhs.line == rhs.line && lhs.col == rhs.col;
}

static bool shouldStopForStep(GxDebuggerStepKind stepKind, const GxDebuggerLocation &stepOrigin,
                              const GxDebuggerLocation &current)
{
    if (stepKind == GxDebuggerStepKind::None || !current.valid) {
        return false;
    }
    if (!stepOrigin.valid) {
        return true;
    }
    if (sameDebuggerSourceLocation(current, stepOrigin)) {
        return false;
    }

    switch (stepKind) {
    case GxDebuggerStepKind::Into:
        return true;
    case GxDebuggerStepKind::Over:
        if (current.frameDepth > stepOrigin.frameDepth) {
            return false;
        }
        return true;
    case GxDebuggerStepKind::Out:
        return current.frameDepth < stepOrigin.frameDepth;
    case GxDebuggerStepKind::None:
        break;
    }
    return false;
}

static GxDebuggerPollSnapshot makeDebuggerPollSnapshot(GxQjsDebuggerState *state,
                                                       const GxDebuggerLocation &current)
{
    GxDebuggerPollSnapshot snapshot;
    if (!state) {
        return snapshot;
    }

    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    snapshot.evaluatingWatches = state->evaluatingWatches;
    snapshot.pauseRequested = state->pauseRequested;
    snapshot.pauseOnException = state->pauseOnException;
    snapshot.printStackOnBreak = state->printStackOnBreak;
    snapshot.interactiveOnBreak = state->interactiveOnBreak;
    snapshot.trapOnBreak = state->trapOnBreak;
    snapshot.stepKind = state->stepKind;
    snapshot.stepOrigin = state->stepOrigin;

    auto suppressedIt = state->suppressedBreakpoints.find(current.frameId);
    if (suppressedIt != state->suppressedBreakpoints.end()
        && (suppressedIt->second.file != current.file || suppressedIt->second.line != current.line
            || current.pcOffset < suppressedIt->second.pcOffset)) {
        state->suppressedBreakpoints.erase(suppressedIt);
        suppressedIt = state->suppressedBreakpoints.end();
    }
    snapshot.suppressed = suppressedIt != state->suppressedBreakpoints.end();
    return snapshot;
}

static void markDebuggerPaused(GxQjsDebuggerState *state, const GxDebuggerLocation &current,
                               GxDebuggerPauseReason pauseReason, bool breakpointPause,
                               GxDebuggerStepKind pauseStepKind,
                               const GxDebuggerLocation &pauseStepOrigin)
{
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    state->pauseRequested = false;
    state->stepKind = GxDebuggerStepKind::None;
    state->stepOrigin = {};
    state->paused = true;
    state->livePaused = true;
    state->pauseReason = pauseReason;
    state->pauseLocation = current;
    state->pauseStepKind = pauseStepKind;
    state->pauseStepOrigin = pauseStepKind != GxDebuggerStepKind::None ? pauseStepOrigin : GxDebuggerLocation{};
    if (breakpointPause) {
        state->suppressedBreakpoints[current.frameId] = {current.file, current.line, current.pcOffset};
    }
    refreshDebuggerFastFlagsLocked(state);
}

static void requestDebuggerStepFromLocation(GxQjsDebuggerState *state, GxDebuggerStepKind kind,
                                            const GxDebuggerLocation &origin)
{
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    state->pauseRequested = false;
    state->resumeRequested = true;
    state->stepKind = kind;
    state->stepOrigin = origin;
    refreshDebuggerFastFlagsLocked(state);
    state->controlCondition.notify_all();
    std::fprintf(stderr, "[GxDebugger] %s\n", stepKindName(kind));
}

static void clearDebuggerStep(GxQjsDebuggerState *state)
{
    std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
    state->stepKind = GxDebuggerStepKind::None;
    state->stepOrigin = {};
    refreshDebuggerFastFlagsLocked(state);
}

static void waitForHostDebuggerCommand(GxQjsDebuggerState *state)
{
    if (!state) {
        return;
    }
    std::unique_lock<std::recursive_mutex> lock(state->syncMutex);
    state->controlCondition.wait(lock, [state] {
        return state->resumeRequested || state->stepKind != GxDebuggerStepKind::None;
    });
    state->resumeRequested = false;
}

static bool printWatchesWithLazyLocals(JSContext *ctx, GxQjsDebuggerState *state,
                                       JSDebuggerLocalsGetter *getLocals, void *getLocalsOpaque)
{
    bool hasWatches = false;
    {
        std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
        hasWatches = !state->watches.empty();
    }
    if (!hasWatches) {
        return true;
    }

    JSValue locals = getLocals(ctx, getLocalsOpaque);
    if (JS_IsException(locals)) {
        JS_FreeValue(ctx, JS_GetException(ctx));
        std::fprintf(stderr, "[GxDebugger] failed to collect locals for watches\n");
        return false;
    }

    printWatches(ctx, state, locals);
    JS_FreeValue(ctx, locals);
    return true;
}

static void printExpressionWithLazyLocals(JSContext *ctx, GxQjsDebuggerState *state,
                                          JSDebuggerLocalsGetter *getLocals, void *getLocalsOpaque,
                                          const std::string &expr)
{
    JSValue locals = getLocals(ctx, getLocalsOpaque);
    if (JS_IsException(locals)) {
        JS_FreeValue(ctx, JS_GetException(ctx));
        std::fprintf(stderr, "[GxDebugger] failed to collect locals for expression\n");
        return;
    }

    printWatchValue(ctx, state, locals, expr);
    JS_FreeValue(ctx, locals);
}

static void printLocals(JSContext *ctx, GxQjsDebuggerState *state, JSValueConst locals)
{
    beginWatchEvaluation(ctx, state);
    JSPropertyEnum *props = nullptr;
    uint32_t length = 0;
    if (JS_GetOwnPropertyNames(ctx, &props, &length, locals, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0) {
        JS_FreeValue(ctx, JS_GetException(ctx));
        std::fprintf(stderr, "[GxDebugger] failed to enumerate locals\n");
        endWatchEvaluation(ctx, state);
        return;
    }

    if (length == 0) {
        std::fprintf(stderr, "[GxDebugger] locals: <empty>\n");
        JS_FreePropertyEnum(ctx, props, length);
        endWatchEvaluation(ctx, state);
        return;
    }

    std::fprintf(stderr, "[GxDebugger] locals:\n");
    for (uint32_t i = 0; i < length; ++i) {
        const char *name = JS_AtomToCString(ctx, props[i].atom);
        if (!name) {
            JS_FreeValue(ctx, JS_GetException(ctx));
            continue;
        }

        JSValue value = JS_GetProperty(ctx, locals, props[i].atom);
        if (JS_IsException(value)) {
            JS_FreeValue(ctx, JS_GetException(ctx));
            std::fprintf(stderr, "  %s = <exception>\n", name);
            JS_FreeCString(ctx, name);
            continue;
        }

        const char *valueStr = JS_ToCString(ctx, value);
        if (valueStr) {
            const std::string valueLine = escapeDebuggerSingleLine(valueStr);
            std::fprintf(stderr, "  %s = %s\n", name, valueLine.empty() ? "\"\"" : valueLine.c_str());
            JS_FreeCString(ctx, valueStr);
        } else {
            JS_FreeValue(ctx, JS_GetException(ctx));
            std::fprintf(stderr, "  %s = <unprintable>\n", name);
        }

        JS_FreeValue(ctx, value);
        JS_FreeCString(ctx, name);
    }

    JS_FreePropertyEnum(ctx, props, length);
    endWatchEvaluation(ctx, state);
}

static void printLocalsWithLazyLocals(JSContext *ctx, GxQjsDebuggerState *state,
                                      JSDebuggerLocalsGetter *getLocals, void *getLocalsOpaque)
{
    JSValue locals = getLocals(ctx, getLocalsOpaque);
    if (JS_IsException(locals)) {
        JS_FreeValue(ctx, JS_GetException(ctx));
        std::fprintf(stderr, "[GxDebugger] failed to collect locals\n");
        return;
    }

    printLocals(ctx, state, locals);
    JS_FreeValue(ctx, locals);
}

static void printInteractiveHelp()
{
    std::fprintf(stderr,
                 "[GxDebugger] help\n"
                 "  Execution:\n"
                 "    c, continue, resume       Continue running until the next breakpoint or pause.\n"
                 "    s, step, stepInto         Step into the next JS source location, entering JS calls.\n"
                 "    n, next, stepOver         Step over JS calls and pause at the next location in this frame.\n"
                 "    finish, out, stepOut      Run until the current JS function returns to its caller.\n"
                 "    q, quit                   Stop execution with a debugger error.\n"
                 "\n"
                 "  Inspection:\n"
                 "    bt, backtrace             Print the current JS call stack.\n"
                 "    locals, vars, scope       Print current frame arguments and local variables.\n"
                 "    args                      Alias for locals; args and locals share one view.\n"
                 "    w, watch, watches         Evaluate all configured watch expressions.\n"
                 "    p <expr>, print <expr>    Evaluate one expression in the current frame locals.\n"
                 "\n"
                 "  Breakpoints:\n"
                 "    lb, bl, breakpoints       List breakpoints with numeric ids.\n"
                 "    info breakpoints          List breakpoints, GDB-style alias.\n"
                 "    b <file>:<line>           Add a breakpoint. Use . for the current paused file.\n"
                 "    b <file>:<line> if <expr> Add a conditional breakpoint.\n"
                 "    break <file>:<line>       Add a breakpoint, long form.\n"
                 "    d <id>                    Delete a breakpoint by id from lb.\n"
                 "    d <file>:<line>           Delete a breakpoint by location. . is supported.\n"
                 "    delete, clear, remove     Aliases for d.\n"
                 "\n"
                 "  Examples:\n"
                 "    b .:42                    Break at line 42 in the current file.\n"
                 "    b .:42 if a > 10          Break only when the condition is truthy.\n"
                 "    b examples/js/test.js:27  Break at a path suffix or full path.\n"
                 "    locals                    Print visible variables at the current stop.\n"
                 "    p self.a + b              Print an expression using current locals.\n"
                 "    d 2                       Delete breakpoint #2.\n");
}

static bool runInteractiveDebugger(JSContext *ctx, GxQjsDebuggerState *state,
                                   JSDebuggerLocalsGetter *getLocals, void *getLocalsOpaque,
                                   const GxDebuggerLocation &current)
{
    flushDebuggerOutput();
    std::fprintf(stderr,
                 "[GxDebugger] interactive mode. Type h or help for commands.\n");

    char input[2048];
    for (;;) {
        {
            std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
            if (state->resumeRequested) {
                break;
            }
        }
        flushDebuggerOutput();
        std::fprintf(stderr, "(gxdbg) ");
        std::fflush(stderr);

        if (!std::fgets(input, sizeof(input), stdin)) {
            {
                std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
                state->resumeRequested = true;
            }
            clearDebuggerStep(state);
            break;
        }

        const std::string command = trimDebuggerCommand(input);
        if (command.empty()) {
            continue;
        }
        if (command == "c" || command == "continue" || command == "resume") {
            GxDebuggerBreakpoint breakpoint;
            if (tryGetBreakpointAt(state, current.file, current.line, &breakpoint)) {
                std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
                state->suppressedBreakpoints[current.frameId] = {current.file, current.line, current.pcOffset};
            }
            {
                std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
                state->resumeRequested = true;
            }
            clearDebuggerStep(state);
            break;
        }
        if (command == "s" || command == "step" || command == "stepInto") {
            requestDebuggerStepFromLocation(state, GxDebuggerStepKind::Into, current);
            break;
        }
        if (command == "n" || command == "next" || command == "stepOver") {
            requestDebuggerStepFromLocation(state, GxDebuggerStepKind::Over, current);
            break;
        }
        if (command == "finish" || command == "out" || command == "stepOut") {
            requestDebuggerStepFromLocation(state, GxDebuggerStepKind::Out, current);
            break;
        }
        if (command == "bt" || command == "backtrace") {
            printCurrentJsStack(ctx, state, &current);
            continue;
        }
        if (command == "locals" || command == "vars" || command == "scope" || command == "args") {
            printLocalsWithLazyLocals(ctx, state, getLocals, getLocalsOpaque);
            continue;
        }
        if (command == "lb" || command == "bl" || command == "breakpoints" || command == "info breakpoints") {
            printBreakpointList(state);
            continue;
        }
        if (command.rfind("b ", 0) == 0 || command.rfind("break ", 0) == 0) {
            const size_t offset = command[0] == 'b' ? 2 : 6;
            const std::string spec = trimDebuggerCommand(command.substr(offset));
            std::string location;
            std::string condition;
            std::string file;
            int line = 0;
            if (!splitBreakpointCondition(spec, &location, &condition)
                || !parseBreakpointSpec(ctx, location, &current.file, &file, &line)) {
                std::fprintf(stderr, "[GxDebugger] breakpoint expects <file>:<line> [if <expr>]\n");
            } else {
                setDebuggerBreakpoint(state, file, line, condition);
                updateDebuggerHandler(ctx, state);
                size_t breakpointId = 0;
                for (const auto &breakpoint: collectBreakpointViews(state)) {
                    if (breakpoint.file == file && breakpoint.line == line) {
                        breakpointId = breakpoint.id;
                        break;
                    }
                }
                std::fprintf(stderr, "[GxDebugger] breakpoint added #%zu %s:%d",
                             breakpointId, file.c_str(), line);
                if (!condition.empty()) {
                    std::fprintf(stderr, " if %s", condition.c_str());
                }
                std::fprintf(stderr, "\n");
            }
            continue;
        }
        if (command.rfind("d ", 0) == 0 || command.rfind("delete ", 0) == 0
            || command.rfind("clear ", 0) == 0 || command.rfind("remove ", 0) == 0) {
            size_t offset = 2;
            if (command.rfind("delete ", 0) == 0) {
                offset = 7;
            } else if (command.rfind("clear ", 0) == 0) {
                offset = 6;
            } else if (command.rfind("remove ", 0) == 0) {
                offset = 7;
            }
            const std::string spec = trimDebuggerCommand(command.substr(offset));
            if (spec.empty()) {
                std::fprintf(stderr, "[GxDebugger] delete expects <id> or <file>:<line>\n");
            } else if (!removeBreakpointBySpec(ctx, state, spec, &current.file)) {
                std::fprintf(stderr, "[GxDebugger] breakpoint not found: %s\n", spec.c_str());
            } else {
                updateDebuggerHandler(ctx, state);
                std::fprintf(stderr, "[GxDebugger] breakpoint removed: %s\n", spec.c_str());
            }
            continue;
        }
        if (command == "w" || command == "watch" || command == "watches") {
            printWatchesWithLazyLocals(ctx, state, getLocals, getLocalsOpaque);
            continue;
        }
        if (command == "q" || command == "quit") {
            {
                std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
                state->resumeRequested = false;
            }
            clearDebuggerStep(state);
            return false;
        }
        if (command == "h" || command == "help") {
            printInteractiveHelp();
            continue;
        }
        if (command.rfind("p ", 0) == 0 || command.rfind("print ", 0) == 0) {
            const size_t offset = command[0] == 'p' ? 2 : 6;
            const std::string expr = trimDebuggerCommand(command.substr(offset));
            if (expr.empty()) {
                std::fprintf(stderr, "[GxDebugger] print expects an expression\n");
            } else {
                printExpressionWithLazyLocals(ctx, state, getLocals, getLocalsOpaque, expr);
            }
            continue;
        }

        std::fprintf(stderr, "[GxDebugger] unknown command: %s\n", command.c_str());
    }

    {
        std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
        state->resumeRequested = false;
    }
    return true;
}

static int gxQjsDebuggerPoll(JSContext *ctx, const char *filename, int lineNum, int colNum,
                             uint64_t frameId, uint32_t frameDepth, uint32_t pcOffset,
                             JSDebuggerPollKind pollKind, JSValueConst,
                             JSDebuggerLocalsGetter *getLocals, void *getLocalsOpaque, void *opaque)
{
    GxQjsDebuggerState *state = static_cast<GxQjsDebuggerState *>(opaque);
    if (!state || lineNum <= 0) {
        return 0;
    }

    const uint32_t fastFlags = state->fastFlags.load(std::memory_order_acquire);
    if (fastFlags & GxDebuggerFastFlag_Evaluating) {
        return 0;
    }
    if (pollKind == JS_DEBUGGER_POLL_EXCEPTION) {
        if (!(fastFlags & GxDebuggerFastFlag_PauseOnException)) {
            return 0;
        }
    } else if (pollKind != JS_DEBUGGER_POLL_DEBUGGER) {
        constexpr uint32_t stoppableFlags = GxDebuggerFastFlag_PauseRequested
                                            | GxDebuggerFastFlag_StepRequested
                                            | GxDebuggerFastFlag_HasBreakpoints;
        if (!(fastFlags & stoppableFlags)) {
            return 0;
        }
    }

    const std::string file = filename ? filename : "<anonymous>";
    const GxDebuggerLocation current = makeDebuggerLocation(file, lineNum, colNum, frameId, frameDepth, pcOffset,
                                                            pollKind);
    const GxDebuggerPollSnapshot snapshot = makeDebuggerPollSnapshot(state, current);
    if (snapshot.evaluatingWatches) {
        return 0;
    }

    const bool pauseHit = snapshot.pauseRequested;
    const bool stepHit = shouldStopForStep(snapshot.stepKind, snapshot.stepOrigin, current);
    const bool exceptionHit = pollKind == JS_DEBUGGER_POLL_EXCEPTION && snapshot.pauseOnException;
    const bool debuggerStatementHit = pollKind == JS_DEBUGGER_POLL_DEBUGGER;
    GxDebuggerBreakpoint breakpoint;
    const bool breakpointConfigured = tryGetBreakpointAt(state, file, lineNum, &breakpoint);
    bool breakpointPause = breakpointConfigured && !snapshot.suppressed && !pauseHit && !stepHit;
    if (breakpointPause) {
        breakpointPause = shouldPauseForBreakpointCondition(ctx, state, &breakpoint, getLocals, getLocalsOpaque);
        if (!breakpointPause) {
            return 0;
        }
    }
    if (!pauseHit && !stepHit && !breakpointPause && !exceptionHit && !debuggerStatementHit) {
        return 0;
    }

    GxDebuggerPauseReason pauseReason = GxDebuggerPauseReason::Breakpoint;
    if (exceptionHit) {
        pauseReason = GxDebuggerPauseReason::Exception;
    } else if (debuggerStatementHit) {
        pauseReason = GxDebuggerPauseReason::DebuggerStatement;
    } else if (pauseHit) {
        pauseReason = GxDebuggerPauseReason::Pause;
    } else if (stepHit) {
        pauseReason = GxDebuggerPauseReason::Step;
    }

    const GxDebuggerStepKind pauseStepKind = stepHit ? snapshot.stepKind : GxDebuggerStepKind::None;
    const GxDebuggerLocation pauseStepOrigin = stepHit ? snapshot.stepOrigin : GxDebuggerLocation{};
    markDebuggerPaused(state, current, pauseReason, breakpointPause, pauseStepKind, pauseStepOrigin);

    if (exceptionHit) {
        std::fprintf(stderr, "[GxDebugger] paused on exception at %s:%d:%d\n", file.c_str(), lineNum, colNum);
    } else if (debuggerStatementHit) {
        std::fprintf(stderr, "[GxDebugger] paused on debugger statement at %s:%d:%d\n",
                     file.c_str(), lineNum, colNum);
    } else {
        std::fprintf(stderr, "[GxDebugger] paused at %s:%d:%d\n", file.c_str(), lineNum, colNum);
    }
    if (snapshot.printStackOnBreak) {
        printCurrentJsStack(ctx, state, &current);
    }
    printWatchesWithLazyLocals(ctx, state, getLocals, getLocalsOpaque);

    const bool interactiveOnBreak = snapshot.interactiveOnBreak;
    const bool trapOnBreak = snapshot.trapOnBreak;
    if (interactiveOnBreak) {
        if (!runInteractiveDebugger(ctx, state, getLocals, getLocalsOpaque, current)) {
            clearPausedState(state);
            return 1;
        }
        clearPausedState(state);
        updateDebuggerHandler(ctx, state);
        return 0;
    }

    if (trapOnBreak) {
        nativeDebugBreak();
        deactivateLivePausedState(state);
        updateDebuggerHandler(ctx, state);
        return 0;
    }

    waitForHostDebuggerCommand(state);
    clearPausedState(state);
    updateDebuggerHandler(ctx, state);
    return 0;
}

static void installGxDebugger(JSContext *ctx, GxQjsDebuggerState *debuggerState)
{
    const JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "debugBreak", JS_NewCFunction(ctx, js_debugBreak, "debugBreak", 0));

    const JSValue debuggerObj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, debuggerObj, "setBreakpoint",
                      JS_NewCFunction(ctx, js_gxDebuggerSetBreakpoint, "setBreakpoint", 3));
    JS_SetPropertyStr(ctx, debuggerObj, "clearBreakpoint",
                      JS_NewCFunction(ctx, js_gxDebuggerClearBreakpoint, "clearBreakpoint", 2));
    JS_SetPropertyStr(ctx, debuggerObj, "clearAllBreakpoints",
                      JS_NewCFunction(ctx, js_gxDebuggerClearAllBreakpoints, "clearAllBreakpoints", 0));
    JS_SetPropertyStr(ctx, debuggerObj, "listBreakpoints",
                      JS_NewCFunction(ctx, js_gxDebuggerListBreakpoints, "listBreakpoints", 0));
    JS_SetPropertyStr(ctx, debuggerObj, "pause",
                      JS_NewCFunction(ctx, js_gxDebuggerPause, "pause", 0));
    JS_SetPropertyStr(ctx, debuggerObj, "isPaused",
                      JS_NewCFunction(ctx, js_gxDebuggerIsPaused, "isPaused", 0));
    JS_SetPropertyStr(ctx, debuggerObj, "getPauseState",
                      JS_NewCFunction(ctx, js_gxDebuggerGetPauseState, "getPauseState", 0));
    JS_SetPropertyStr(ctx, debuggerObj, "resume",
                      JS_NewCFunction(ctx, js_gxDebuggerResume, "resume", 0));
    JS_SetPropertyStr(ctx, debuggerObj, "stepInto",
                      JS_NewCFunction(ctx, js_gxDebuggerStepInto, "stepInto", 0));
    JS_SetPropertyStr(ctx, debuggerObj, "stepOver",
                      JS_NewCFunction(ctx, js_gxDebuggerStepOver, "stepOver", 0));
    JS_SetPropertyStr(ctx, debuggerObj, "stepOut",
                      JS_NewCFunction(ctx, js_gxDebuggerStepOut, "stepOut", 0));
    JS_SetPropertyStr(ctx, debuggerObj, "setTrapOnBreak",
                      JS_NewCFunction(ctx, js_gxDebuggerSetTrapOnBreak, "setTrapOnBreak", 1));
    JS_SetPropertyStr(ctx, debuggerObj, "setInteractiveOnBreak",
                      JS_NewCFunction(ctx, js_gxDebuggerSetInteractiveOnBreak, "setInteractiveOnBreak", 1));
    JS_SetPropertyStr(ctx, debuggerObj, "setPrintStackOnBreak",
                      JS_NewCFunction(ctx, js_gxDebuggerSetPrintStackOnBreak, "setPrintStackOnBreak", 1));
    JS_SetPropertyStr(ctx, debuggerObj, "setPauseOnException",
                      JS_NewCFunction(ctx, js_gxDebuggerSetPauseOnException, "setPauseOnException", 1));
    JS_SetPropertyStr(ctx, debuggerObj, "watch",
                      JS_NewCFunction(ctx, js_gxDebuggerWatch, "watch", 1));
    JS_SetPropertyStr(ctx, debuggerObj, "clearWatch",
                      JS_NewCFunction(ctx, js_gxDebuggerClearWatch, "clearWatch", 1));
    JS_SetPropertyStr(ctx, debuggerObj, "clearAllWatches",
                      JS_NewCFunction(ctx, js_gxDebuggerClearAllWatches, "clearAllWatches", 0));
    JS_SetPropertyStr(ctx, global, "GxDebugger", debuggerObj);
    JS_FreeValue(ctx, global);
    updateDebuggerHandler(ctx, debuggerState);
}

static bool isGAnyModuleName(const char *moduleName)
{
    return moduleName && std::string(moduleName).rfind(GAnyModulePrefix, 0) == 0;
}

static bool hasJsModuleSyntax(const std::string &script)
{
    bool atLineStart = true;

    for (size_t i = 0; i < script.size();) {
        const char c = script[i];

        if (c == '\n' || c == '\r') {
            atLineStart = true;
            ++i;
            continue;
        }
        if (atLineStart && (c == ' ' || c == '\t')) {
            ++i;
            continue;
        }
        if (c == '/' && i + 1 < script.size() && script[i + 1] == '/') {
            i += 2;
            while (i < script.size() && script[i] != '\n' && script[i] != '\r') {
                ++i;
            }
            continue;
        }
        if (c == '/' && i + 1 < script.size() && script[i + 1] == '*') {
            i += 2;
            while (i + 1 < script.size() && !(script[i] == '*' && script[i + 1] == '/')) {
                if (script[i] == '\n' || script[i] == '\r') {
                    atLineStart = true;
                }
                ++i;
            }
            if (i + 1 < script.size()) {
                i += 2;
            }
            continue;
        }

        if (atLineStart && (script.compare(i, 6, "import") == 0 || script.compare(i, 6, "export") == 0)) {
            const size_t nextIndex = i + 6;
            const bool isIdentifierTail = nextIndex < script.size()
                                          && (std::isalnum(static_cast<unsigned char>(script[nextIndex])) || script[nextIndex] == '_' || script[nextIndex] == '$');
            if (!isIdentifierTail) {
                size_t tokenIndex = nextIndex;
                while (tokenIndex < script.size() && (script[tokenIndex] == ' ' || script[tokenIndex] == '\t')) {
                    ++tokenIndex;
                }
                if (script.compare(i, 6, "export") == 0 || tokenIndex == script.size() || script[tokenIndex] != '(') {
                    return true;
                }
            }
        }

        atLineStart = false;
        if (c == '\'' || c == '"' || c == '`') {
            const char quote = c;
            ++i;
            while (i < script.size()) {
                if (script[i] == '\\') {
                    i += 2;
                    continue;
                }
                if (script[i] == quote) {
                    ++i;
                    break;
                }
                ++i;
            }
            continue;
        }
        ++i;
    }

    return false;
}

static bool shouldEvalAsJsModule(const std::string &script, const std::string &sourcePath)
{
    const GString path = sourcePath;
    return path.toLower().endWith(".mjs") || hasJsModuleSyntax(script);
}

static GAnyModuleExports getGAnyModuleExports(const std::string &moduleName)
{
    GAnyModuleExports exports;

    if (moduleName.rfind(GAnyModulePrefix, 0) != 0) {
        return exports;
    }

    const std::string importPath = moduleName.substr(std::string(GAnyModulePrefix).size());
    if (importPath.empty()) {
        return exports;
    }

    const bool isWildcardPath = importPath.size() >= 2 && importPath.substr(importPath.size() - 2) == ".*";

    GAny imported = GAny::Import(importPath);
    if (imported.isNull() && !isWildcardPath) {
        imported = GAny::Import(importPath + ".*");
    }

    if (imported.isNull()) {
        return exports;
    }

    exports.valid = true;
    exports.defaultValue = imported;

    std::set<std::string> exportedNames;
    if (imported.isObject()) {
        auto it = imported.iterator();
        while (it.hasNext()) {
            auto item = it.next();
            const std::string name = item.first.castAs<std::string>();
            if (!name.empty() && exportedNames.insert(name).second) {
                exports.namedExports.emplace_back(name, item.second);
            }
        }
    } else if (imported.isClass()) {
        const auto *clazz = imported.as<GAnyClass>();
        if (clazz && !clazz->getName().empty() && exportedNames.insert(clazz->getName()).second) {
            exports.namedExports.emplace_back(clazz->getName(), imported);
        }
    }

    return exports;
}

static std::string getModuleName(JSContext *ctx, JSModuleDef *m)
{
    const JSAtom atom = JS_GetModuleName(ctx, m);
    const JSValue value = JS_AtomToValue(ctx, atom);
    const char *str = JS_ToCString(ctx, value);
    std::string moduleName = str ? str : "";

    JS_FreeCString(ctx, str);
    JS_FreeValue(ctx, value);
    JS_FreeAtom(ctx, atom);

    return moduleName;
}

static int JS_ganyModuleInit(JSContext *ctx, JSModuleDef *m)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAnyModuleExports exports = getGAnyModuleExports(getModuleName(ctx, m));
    if (!exports.valid) {
        JS_ThrowReferenceError(ctx, "Could not load GAny module");
        return -1;
    }

    JS_SetModuleExport(ctx, m, "default", GAnyToQJS::makeGAnyToJsValue(jsState, exports.defaultValue, exports.defaultValue.isObject()));
    for (const auto &item: exports.namedExports) {
        JS_SetModuleExport(ctx, m, item.first.c_str(), GAnyToQJS::makeGAnyToJsValue(jsState, item.second, item.second.isObject()));
    }

    return 0;
}
}


static void jsStateObjectFinalizer(JSRuntime *rt, JSValue val)
{
    const JSClassID classId = JS_GetClassID(val);

    JS_State *state = static_cast<JS_State *>(JS_GetOpaque(val, classId));
    if (state) {
        JS_SetDebuggerHandler(rt, nullptr, nullptr);
        JS_SetDebuggerStatementHandler(rt, nullptr, nullptr);
        GAnyToQJS::releaseJS(state);
        GxQjsDebuggerState *debuggerState = static_cast<GxQjsDebuggerState *>(state->debuggerState);
        GX_DELETE(debuggerState);
        state->debuggerState = nullptr;
        GX_DELETE(state);
    }
}

JSContext * JS_NewCustomContext(JSRuntime *rt, bool isWorker)
{
    JS_SetModuleLoaderFunc(rt, GAnyJSImplQjs::JS_moduleNormalizeFunc, GAnyJSImplQjs::JS_moduleLoader, nullptr);

    JSContext *ctx = JS_NewContext(rt);
    if (!ctx) {
        return nullptr;
    }

    /* system modules */
    js_init_module_std(ctx, "qjs:std");
    js_init_module_os(ctx, "qjs:os");
    js_init_module_bjson(ctx, "qjs:bjson");

    js_std_add_helpers(ctx, 0, nullptr);

    JS_State *jsState = GX_NEW(JS_State);

    jsState->rt = rt;
    jsState->ctx = ctx;
    jsState->threadId = GThread::currentThreadId();
    jsState->debuggerState = GX_NEW(GxQjsDebuggerState);

    GAnyToQJS::toJS(jsState, isWorker);

    JS_SetContextOpaque(ctx, jsState);

    // Hosting the lifecycle for JSContext
    constexpr JSClassDef classDef{
        .class_name = "JsState",
        .finalizer = jsStateObjectFinalizer
    };

    JSClassID stateClassId = 0;
    JS_NewClassID(rt, &stateClassId);

    JS_NewClass(rt, stateClassId, &classDef);

    const JSValue jsStateObj = JS_NewObjectClass(ctx, stateClassId);
    GX_ASSERT(JS_IsObject(jsStateObj));

    JS_SetOpaque(jsStateObj, jsState);

    const JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "JSState", jsStateObj);
    JS_FreeValue(ctx, global);

    installGxDebugger(ctx, static_cast<GxQjsDebuggerState *>(jsState->debuggerState));

    return ctx;
}

JSContext *JS_NewWorkerContext(JSRuntime *rt)
{
    return JS_NewCustomContext(rt, true);
}

// ================================================================

static GAnyJS::FileReader sDefFileReader = [](const std::string &path) -> GByteArray {
    const GString tPath = path;

    GFile file(path);

    GFile::OpenModeFlags openModeFlags = GFile::ReadOnly;
    if (!tPath.toLower().endWith(".js")) {
        openModeFlags |= GFile::Binary;
    }

    GByteArray bytes;
    if (file.open(openModeFlags)) {
        bytes = file.read();
        file.close();
    }
    return bytes;
};

static GAnyJS::ModuleNameNormalizeFunc sDefModuleNormalizeFunc = [](const std::string &moduleBasePath, const std::string &moduleName) -> std::string {
    GFile baseDir(moduleBasePath);
    if (!baseDir.isDirectory()) {
        baseDir = baseDir.parent();
    }

    GFile moduleFile(baseDir, moduleName);
    if (moduleFile.fileSuffix().empty()) {
        moduleFile = GFile(baseDir, moduleName + ".js");
    }
    return moduleFile.absoluteFilePath();
};

static GAnyJS::ExceptionHandler sDefExceptionHandler = [](const std::string &msg) {
    fprintf(stderr, "%s\n", msg.c_str());
};

GAnyJS::FileReader GAnyJSImplQjs::sFileReader = sDefFileReader;
GAnyJS::ModuleNameNormalizeFunc GAnyJSImplQjs::sModuleNormalizeFunc = sDefModuleNormalizeFunc;
GAnyJS::ExceptionHandler GAnyJSImplQjs::sExceptionHandler = sDefExceptionHandler;


std::shared_ptr<GAnyJS> GAnyJS::threadLocal()
{
    thread_local auto vm = std::make_shared<GAnyJSImplQjs>();
    return vm;
}

void GAnyJS::setFileReader(FileReader reader)
{
    if (reader) {
        GAnyJSImplQjs::sFileReader = std::move(reader);
    } else {
        GAnyJSImplQjs::sFileReader = sDefFileReader;
    }
}

void GAnyJS::setModuleNameNormalizeFunc(ModuleNameNormalizeFunc func)
{
    if (func) {
        GAnyJSImplQjs::sModuleNormalizeFunc = std::move(func);
    } else {
        GAnyJSImplQjs::sModuleNormalizeFunc = sDefModuleNormalizeFunc;
    }
}

void GAnyJS::setExceptionHandler(ExceptionHandler handler)
{
    if (handler) {
        GAnyJSImplQjs::sExceptionHandler = std::move(handler);
    } else {
        GAnyJSImplQjs::sExceptionHandler = sDefExceptionHandler;
    }
}

GAnyJSImplQjs::GAnyJSImplQjs()
    : mJsRuntime(nullptr),
      mJSContext(nullptr),
      mJSState(nullptr)
{
    init();
}

GAnyJSImplQjs::~GAnyJSImplQjs()
{
    shutdownImpl();
}

void GAnyJSImplQjs::shutdown()
{
    shutdownImpl();
}

void GAnyJSImplQjs::gc()
{
    CHECK_CONDITION_V(mJsRuntime != nullptr);

    JS_RunGC(mJsRuntime);
}

bool GAnyJSImplQjs::onUpdate()
{
    CHECK_CONDITION_R(mJSContext != nullptr, false);

    const int r = js_std_loop(mJSContext);
    if (r && sExceptionHandler) {
        const JSValue exceptionVal = JS_GetException(mJSContext);

        std::string exceptionMsg;

        JSValue val;
        const bool isError = JS_IsError(exceptionVal);

        const char *str = JS_ToCString(mJSContext, exceptionVal);
        if (str) {
            exceptionMsg += std::string(str);
            JS_FreeCString(mJSContext, str);
        }

        if (isError) {
            val = JS_GetPropertyStr(mJSContext, exceptionVal, "stack");
        } else {
            js_std_cmd(/*ErrorBackTrace*/2, mJSContext, &val);
        }
        if (!JS_IsUndefined(val)) {
            exceptionMsg += "\n";
            str = JS_ToCString(mJSContext, val);
            if (str) {
                exceptionMsg += std::string(str);
                JS_FreeCString(mJSContext, str);
            }
            JS_FreeValue(mJSContext, val);
        }
        JS_FreeValue(mJSContext, exceptionVal);

        sExceptionHandler(exceptionMsg);
    }

    return r == 0;
}

GAny GAnyJSImplQjs::eval(const std::string &script, const std::string &sourcePath, const GAny &env)
{
    CHECK_CONDITION_R(mJSContext != nullptr, GAnyException("JS runtime not initialized"));

    JSContext *ctx = mJSContext;
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));

    if (shouldEvalAsJsModule(script, sourcePath)) {
        GAny envObj;
        if (env.isObject()) {
            envObj = env;
        } else {
            envObj = GAny::object();
        }

        const JSValue global = JS_GetGlobalObject(ctx);
        const JSAtom envAtom = JS_NewAtom(ctx, "Env");
        const int hasOldEnv = JS_HasProperty(ctx, global, envAtom);
        JSValue oldEnv = JS_UNDEFINED;
        if (hasOldEnv > 0) {
            oldEnv = JS_GetProperty(ctx, global, envAtom);
        }

        JS_SetProperty(ctx, global, envAtom, GAnyToQJS::makeGAnyToJsValue(jsState, envObj, true));

        JSValue funcObj = JS_Eval(ctx, script.c_str(), script.size(),
                                  sourcePath.empty() ? "<input>" : sourcePath.c_str(),
                                  JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
        if (!JS_IsException(funcObj) && js_module_set_import_meta(ctx, funcObj, false, false) < 0) {
            JS_FreeValue(ctx, funcObj);
            funcObj = JS_EXCEPTION;
        }

        JSValue r = JS_EXCEPTION;
        if (!JS_IsException(funcObj)) {
            r = JS_EvalFunction(ctx, funcObj);
            r = js_std_await(ctx, r);
        }

        GAny result = GAnyToQJS::makeJsValueToGAny(jsState, JS_IsException(funcObj) ? funcObj : r);

        JS_FreeValue(ctx, r);
        if (hasOldEnv > 0) {
            JS_SetProperty(ctx, global, envAtom, oldEnv);
        } else {
            JS_FreeValue(ctx, oldEnv);
            JS_DeleteProperty(ctx, global, envAtom, 0);
        }
        JS_FreeAtom(ctx, envAtom);
        JS_FreeValue(ctx, global);

        js_std_loop(ctx);

        return result;
    }

    const JSValue funcObj = JS_Eval(ctx, script.c_str(), script.size(),
                        sourcePath.empty() ? "<input>" : sourcePath.c_str(),
                        JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(funcObj)) {
        GAny exception = GAnyToQJS::makeJsValueToGAny(jsState, funcObj);
        JS_FreeValue(ctx, funcObj);
        return exception;
    }

    GAny envObj;
    if (env.isObject()) {
        envObj = env;
    } else {
        envObj = GAny::object();
    }

    const JSValue global = JS_GetGlobalObject(ctx);

    JSValue argv[] = {
        GAnyToQJS::makeGAnyToJsValue(jsState, envObj, true)
    };
    const JSValue r = JS_Call(ctx, funcObj, global, 1, argv);

    GAny result = GAnyToQJS::makeJsValueToGAny(jsState, r);

    JS_FreeValue(ctx, r);
    JS_FreeValue(ctx, global);
    JS_FreeValue(ctx, funcObj);
    JS_FreeValue(ctx, argv[0]);

    js_std_loop(ctx);

    return result;
}

GAny GAnyJSImplQjs::evalByteCode(const GByteArray &bytes, const GAny &env)
{
    CHECK_CONDITION_R(mJSContext != nullptr, GAnyException("JS runtime not initialized"));

    JSContext *ctx = mJSContext;
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));

    const JSValue bytecodeFunc = JS_ReadObject(ctx, bytes.data(), bytes.size(), JS_READ_OBJ_BYTECODE);

    if (JS_IsException(bytecodeFunc)) {
        GAny e = GAnyToQJS::makeJsValueToGAny(jsState, bytecodeFunc);
        JS_FreeValue(ctx, bytecodeFunc);
        return e;
    }

    const JSValue funcObj = JS_EvalFunction(ctx, bytecodeFunc);
    if (JS_IsException(funcObj)) {
        GAny e = GAnyToQJS::makeJsValueToGAny(jsState, funcObj);
        JS_FreeValue(ctx, funcObj);
        return e;
    }
    if (!JS_IsFunction(ctx, funcObj)) {
        GAny e = GAnyException("It's not a function");
        JS_FreeValue(ctx, funcObj);
        return e;
    }

    GAny envObj;
    if (env.isObject()) {
        envObj = env;
    } else {
        envObj = GAny::object();
    }

    const JSValue global = JS_GetGlobalObject(mJSContext);

    JSValue argv[] = {
        GAnyToQJS::makeGAnyToJsValue(jsState, envObj, false)
    };
    const JSValue r = JS_Call(mJSContext, funcObj, global, 1, argv);

    GAny result = GAnyToQJS::makeJsValueToGAny(jsState, r);

    JS_FreeValue(mJSContext, r);
    JS_FreeValue(mJSContext, global);
    JS_FreeValue(mJSContext, funcObj);
    JS_FreeValue(mJSContext, argv[0]);

    js_std_loop(mJSContext);

    return result;
}

GAny GAnyJSImplQjs::evalFile(const std::string &filePath, const GAny &env)
{
    CHECK_CONDITION_R(mJSContext != nullptr, GAnyException("JS runtime not initialized"));

    const GFile file(filePath);

    const GByteArray bytes = sFileReader(filePath);
    if (bytes.isEmpty()) {
        return GAnyException("Failed to read file");
    }

    const GString suffix = GString(file.fileSuffix()).toLower();
    if (suffix == "js" || suffix == "mjs") {
        const std::string content(reinterpret_cast<const char *>(bytes.data()), bytes.size());
        return eval(content, file.absoluteFilePath(), env);
    }
    return evalByteCode(bytes, env);
}

GAny GAnyJSImplQjs::compile(const std::string &script, const std::string &sourcePath)
{
    CHECK_CONDITION_R(mJSContext != nullptr, GAnyException("JS runtime not initialized"));

    JSContext *ctx = mJSContext;
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));

    const JSValue funcObj = JS_Eval(ctx, script.c_str(), script.size(), sourcePath.c_str(),
                                    JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);

    if (JS_IsException(funcObj)) {
        GAny exception = GAnyToQJS::makeJsValueToGAny(jsState, funcObj);
        JS_FreeValue(ctx, funcObj);
        return exception;
    }

    size_t bytecodeLen;
    uint8_t *bytecodeBuf = JS_WriteObject(ctx, &bytecodeLen, funcObj, JS_WRITE_OBJ_BYTECODE);

    if (!bytecodeBuf) {
        JS_FreeValue(ctx, funcObj);
        return GAnyException("Failed to write bytecode");
    }

    GByteArray result(bytecodeBuf, bytecodeLen);

    JS_FreeValue(ctx, funcObj);
    js_free(ctx, bytecodeBuf);

    return result;
}

// ================================================================

GAny GAnyJSImplQjs::setBreakpoint(const std::string &file, int line, const std::string &condition)
{
    CHECK_CONDITION_R(mJSContext != nullptr, GAnyException("JS runtime not initialized"));
    CHECK_CONDITION_R(line > 0, GAnyException("breakpoint line must be positive"));

    JS_State *jsState = mJSState;
    CHECK_CONDITION_R(jsState != nullptr, GAnyException("JS state not initialized"));
    CHECK_CONDITION_R(isDebuggerOwnerThread(jsState),
                      GAnyException("setBreakpoint must be called from the JS owning thread"));

    GxQjsDebuggerState *state = getDebuggerState(jsState);
    CHECK_CONDITION_R(state != nullptr, GAnyException("debugger state is not initialized"));

    std::string resolvedFile;
    if (!resolveHostBreakpointFileToken(mJSContext, state, file, &resolvedFile)) {
        return GAnyException("could not resolve current source file for breakpoint");
    }

    setDebuggerBreakpoint(state, resolvedFile, line, trimDebuggerCommand(condition));
    updateDebuggerHandler(mJSContext, state);
    return GAny::undefined();
}

GAny GAnyJSImplQjs::clearBreakpoint(const std::string &file, int line)
{
    CHECK_CONDITION_R(mJSContext != nullptr, GAnyException("JS runtime not initialized"));
    CHECK_CONDITION_R(line > 0, GAnyException("breakpoint line must be positive"));

    JS_State *jsState = mJSState;
    CHECK_CONDITION_R(jsState != nullptr, GAnyException("JS state not initialized"));
    CHECK_CONDITION_R(isDebuggerOwnerThread(jsState),
                      GAnyException("clearBreakpoint must be called from the JS owning thread"));

    GxQjsDebuggerState *state = getDebuggerState(jsState);
    CHECK_CONDITION_R(state != nullptr, GAnyException("debugger state is not initialized"));

    std::string resolvedFile;
    if (!resolveHostBreakpointFileToken(mJSContext, state, file, &resolvedFile)) {
        return GAnyException("could not resolve current source file for breakpoint");
    }

    clearDebuggerBreakpoint(state, resolvedFile, line);
    updateDebuggerHandler(mJSContext, state);
    return GAny::undefined();
}

GAny GAnyJSImplQjs::clearAllBreakpoints()
{
    CHECK_CONDITION_R(mJSContext != nullptr, GAnyException("JS runtime not initialized"));

    JS_State *jsState = mJSState;
    CHECK_CONDITION_R(jsState != nullptr, GAnyException("JS state not initialized"));
    CHECK_CONDITION_R(isDebuggerOwnerThread(jsState),
                      GAnyException("clearAllBreakpoints must be called from the JS owning thread"));

    GxQjsDebuggerState *state = getDebuggerState(jsState);
    CHECK_CONDITION_R(state != nullptr, GAnyException("debugger state is not initialized"));

    clearAllDebuggerBreakpoints(state);
    updateDebuggerHandler(mJSContext, state);
    return GAny::undefined();
}

GAny GAnyJSImplQjs::listBreakpoints() const
{
    CHECK_CONDITION_R(mJSState != nullptr, GAnyException("JS state not initialized"));

    const JS_State *jsState = mJSState;
    CHECK_CONDITION_R(jsState != nullptr, GAnyException("JS state not initialized"));

    return buildDebuggerBreakpointListObject(getDebuggerState(jsState));
}

GAny GAnyJSImplQjs::pause()
{
    CHECK_CONDITION_R(mJSState != nullptr, GAnyException("JS state not initialized"));

    JS_State *jsState = mJSState;
    CHECK_CONDITION_R(jsState != nullptr, GAnyException("JS state not initialized"));

    GxQjsDebuggerState *state = getDebuggerState(jsState);
    CHECK_CONDITION_R(state != nullptr, GAnyException("debugger state is not initialized"));

    requestDebuggerPause(state);
    if (isDebuggerOwnerThread(jsState) && mJSContext) {
        updateDebuggerHandler(mJSContext, state);
    }
    return GAny::undefined();
}

GAny GAnyJSImplQjs::resume()
{
    CHECK_CONDITION_R(mJSState != nullptr, GAnyException("JS state not initialized"));

    JS_State *jsState = mJSState;
    CHECK_CONDITION_R(jsState != nullptr, GAnyException("JS state not initialized"));

    GxQjsDebuggerState *state = getDebuggerState(jsState);
    CHECK_CONDITION_R(state != nullptr, GAnyException("debugger state is not initialized"));

    bool canWakePausedThread = false;
    {
        std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
        canWakePausedThread = state->livePaused && !state->interactiveOnBreak;
    }
    CHECK_CONDITION_R(isDebuggerOwnerThread(jsState) || canWakePausedThread,
                      GAnyException("debugger control must be called from the JS owning thread unless waiting in a non-interactive pause"));

    requestDebuggerResume(state);
    if (isDebuggerOwnerThread(jsState)) {
        updateDebuggerHandler(mJSContext, state);
    }
    return GAny::undefined();
}

GAny GAnyJSImplQjs::stepInto()
{
    CHECK_CONDITION_R(mJSState != nullptr, GAnyException("JS state not initialized"));

    JS_State *jsState = mJSState;
    CHECK_CONDITION_R(jsState != nullptr, GAnyException("JS state not initialized"));

    GxQjsDebuggerState *state = getDebuggerState(jsState);
    CHECK_CONDITION_R(state != nullptr, GAnyException("debugger state is not initialized"));

    bool canWakePausedThread = false;
    {
        std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
        canWakePausedThread = state->livePaused && !state->interactiveOnBreak;
    }
    CHECK_CONDITION_R(isDebuggerOwnerThread(jsState) || canWakePausedThread,
                      GAnyException("debugger control must be called from the JS owning thread unless waiting in a non-interactive pause"));

    requestDebuggerStep(state, GxDebuggerStepKind::Into);
    if (isDebuggerOwnerThread(jsState)) {
        updateDebuggerHandler(mJSContext, state);
    }
    return GAny::undefined();
}

GAny GAnyJSImplQjs::stepOver()
{
    CHECK_CONDITION_R(mJSState != nullptr, GAnyException("JS state not initialized"));

    JS_State *jsState = mJSState;
    CHECK_CONDITION_R(jsState != nullptr, GAnyException("JS state not initialized"));

    GxQjsDebuggerState *state = getDebuggerState(jsState);
    CHECK_CONDITION_R(state != nullptr, GAnyException("debugger state is not initialized"));

    bool canWakePausedThread = false;
    {
        std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
        canWakePausedThread = state->livePaused && !state->interactiveOnBreak;
    }
    CHECK_CONDITION_R(isDebuggerOwnerThread(jsState) || canWakePausedThread,
                      GAnyException("debugger control must be called from the JS owning thread unless waiting in a non-interactive pause"));

    requestDebuggerStep(state, GxDebuggerStepKind::Over);
    if (isDebuggerOwnerThread(jsState)) {
        updateDebuggerHandler(mJSContext, state);
    }
    return GAny::undefined();
}

GAny GAnyJSImplQjs::stepOut()
{
    CHECK_CONDITION_R(mJSState != nullptr, GAnyException("JS state not initialized"));

    JS_State *jsState = mJSState;
    CHECK_CONDITION_R(jsState != nullptr, GAnyException("JS state not initialized"));

    GxQjsDebuggerState *state = getDebuggerState(jsState);
    CHECK_CONDITION_R(state != nullptr, GAnyException("debugger state is not initialized"));

    bool canWakePausedThread = false;
    {
        std::lock_guard<std::recursive_mutex> lock(state->syncMutex);
        canWakePausedThread = state->livePaused && !state->interactiveOnBreak;
    }
    CHECK_CONDITION_R(isDebuggerOwnerThread(jsState) || canWakePausedThread,
                      GAnyException("debugger control must be called from the JS owning thread unless waiting in a non-interactive pause"));

    requestDebuggerStep(state, GxDebuggerStepKind::Out);
    if (isDebuggerOwnerThread(jsState)) {
        updateDebuggerHandler(mJSContext, state);
    }
    return GAny::undefined();
}

GAny GAnyJSImplQjs::setTrapOnBreak(bool enabled)
{
    CHECK_CONDITION_R(mJSState != nullptr, GAnyException("JS state not initialized"));

    JS_State *jsState = mJSState;
    CHECK_CONDITION_R(jsState != nullptr, GAnyException("JS state not initialized"));

    GxQjsDebuggerState *state = getDebuggerState(jsState);
    CHECK_CONDITION_R(state != nullptr, GAnyException("debugger state is not initialized"));

    setDebuggerTrapOnBreak(state, enabled);
    return GAny::undefined();
}

GAny GAnyJSImplQjs::setInteractiveOnBreak(bool enabled)
{
    CHECK_CONDITION_R(mJSState != nullptr, GAnyException("JS state not initialized"));

    JS_State *jsState = mJSState;
    CHECK_CONDITION_R(jsState != nullptr, GAnyException("JS state not initialized"));

    GxQjsDebuggerState *state = getDebuggerState(jsState);
    CHECK_CONDITION_R(state != nullptr, GAnyException("debugger state is not initialized"));

    setDebuggerInteractiveOnBreak(state, enabled);
    return GAny::undefined();
}

GAny GAnyJSImplQjs::setPrintStackOnBreak(bool enabled)
{
    CHECK_CONDITION_R(mJSState != nullptr, GAnyException("JS state not initialized"));

    JS_State *jsState = mJSState;
    CHECK_CONDITION_R(jsState != nullptr, GAnyException("JS state not initialized"));

    GxQjsDebuggerState *state = getDebuggerState(jsState);
    CHECK_CONDITION_R(state != nullptr, GAnyException("debugger state is not initialized"));

    setDebuggerPrintStackOnBreak(state, enabled);
    return GAny::undefined();
}

GAny GAnyJSImplQjs::setPauseOnException(bool enabled)
{
    CHECK_CONDITION_R(mJSContext != nullptr, GAnyException("JS runtime not initialized"));

    JS_State *jsState = mJSState;
    CHECK_CONDITION_R(jsState != nullptr, GAnyException("JS state not initialized"));
    CHECK_CONDITION_R(isDebuggerOwnerThread(jsState),
                      GAnyException("setPauseOnException must be called from the JS owning thread"));

    GxQjsDebuggerState *state = getDebuggerState(jsState);
    CHECK_CONDITION_R(state != nullptr, GAnyException("debugger state is not initialized"));

    setDebuggerPauseOnException(state, enabled);
    updateDebuggerHandler(mJSContext, state);
    return GAny::undefined();
}

GAny GAnyJSImplQjs::watch(const std::string &expression)
{
    CHECK_CONDITION_R(mJSState != nullptr, GAnyException("JS state not initialized"));

    JS_State *jsState = mJSState;
    CHECK_CONDITION_R(jsState != nullptr, GAnyException("JS state not initialized"));

    GxQjsDebuggerState *state = getDebuggerState(jsState);
    CHECK_CONDITION_R(state != nullptr, GAnyException("debugger state is not initialized"));

    addDebuggerWatch(state, expression);
    return GAny::undefined();
}

GAny GAnyJSImplQjs::clearWatch(const std::string &expression)
{
    CHECK_CONDITION_R(mJSState != nullptr, GAnyException("JS state not initialized"));

    JS_State *jsState = mJSState;
    CHECK_CONDITION_R(jsState != nullptr, GAnyException("JS state not initialized"));

    GxQjsDebuggerState *state = getDebuggerState(jsState);
    CHECK_CONDITION_R(state != nullptr, GAnyException("debugger state is not initialized"));

    clearDebuggerWatch(state, expression);
    return GAny::undefined();
}

GAny GAnyJSImplQjs::clearAllWatches()
{
    CHECK_CONDITION_R(mJSState != nullptr, GAnyException("JS state not initialized"));

    JS_State *jsState = mJSState;
    CHECK_CONDITION_R(jsState != nullptr, GAnyException("JS state not initialized"));

    GxQjsDebuggerState *state = getDebuggerState(jsState);
    CHECK_CONDITION_R(state != nullptr, GAnyException("debugger state is not initialized"));

    clearAllDebuggerWatches(state);
    return GAny::undefined();
}

// ================================================================

GAny GAnyJSImplQjs::getPauseState() const
{
    CHECK_CONDITION_R(mJSState != nullptr, GAnyException("JS state not initialized"));

    const JS_State *jsState = mJSState;
    CHECK_CONDITION_R(jsState != nullptr, GAnyException("JS state not initialized"));

    return buildDebuggerPauseStateObject(getDebuggerState(jsState));
}

// ================================================================

const JS_State * GAnyJSImplQjs::getJSState() const
{
    CHECK_CONDITION_R(mJSState != nullptr, nullptr);

    return mJSState;
}

JSModuleDef *GAnyJSImplQjs::JS_moduleLoader(JSContext *ctx, const char *moduleName, void *)
{
    if (isGAnyModuleName(moduleName)) {
        const GAnyModuleExports exports = getGAnyModuleExports(moduleName);
        if (!exports.valid) {
            JS_ThrowReferenceError(ctx, "Could not load GAny module '%s'", moduleName);
            return nullptr;
        }

        JSModuleDef *m = JS_NewCModule(ctx, moduleName, JS_ganyModuleInit);
        if (!m) {
            return nullptr;
        }

        if (JS_AddModuleExport(ctx, m, "default") < 0) {
            return nullptr;
        }
        for (const auto &item: exports.namedExports) {
            if (JS_AddModuleExport(ctx, m, item.first.c_str()) < 0) {
                return nullptr;
            }
        }

        return m;
    }

    const GByteArray buf = sFileReader(moduleName);
    if (buf.isEmpty()) {
        JS_ThrowReferenceError(ctx, "Could not load module filename '%s'", moduleName);
        return nullptr;
    }

    /* compile the module */
    const JSValue funcVal = JS_Eval(ctx, reinterpret_cast<const char *>(buf.data()), buf.size(), moduleName,
                                    JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    if (JS_IsException(funcVal))
        return nullptr;

    if (js_module_set_import_meta(ctx, funcVal, false, false) < 0) {
        JS_FreeValue(ctx, funcVal);
        return nullptr;
    }

    /* the module is already referenced, so we must free it */
    JSModuleDef *m = static_cast<JSModuleDef *>(JS_VALUE_GET_PTR(funcVal));
    JS_FreeValue(ctx, funcVal);

    return m;
}

char *GAnyJSImplQjs::JS_moduleNormalizeFunc(JSContext *ctx, const char *moduleBaseName, const char *moduleName, void *)
{
    if (moduleName[0] != '.') {
        /* if no initial dot, the module name is not modified */
        return js_strdup(ctx, moduleName);
    }

    const std::string normPath = sModuleNormalizeFunc(moduleBaseName, moduleName);

    char *rPath = js_strdup(ctx, normPath.c_str());

    return rPath;
}

// ================================================================

void GAnyJSImplQjs::init()
{
    static bool sInitedQJS = false;
    if (!sInitedQJS) {
        js_std_set_worker_new_context_func(JS_NewWorkerContext);
        sInitedQJS = true;
    }

    mJsRuntime = JS_NewRuntime();

    js_std_init_handlers(mJsRuntime);

    /* exit on unhandled promise rejections */
    JS_SetHostPromiseRejectionTracker(mJsRuntime, js_std_promise_rejection_tracker, nullptr);

    mJSContext = JS_NewCustomContext(mJsRuntime);
    mJSState = mJSContext ? static_cast<JS_State *>(JS_GetContextOpaque(mJSContext)) : nullptr;
}

void GAnyJSImplQjs::shutdownImpl()
{
    if (mJsRuntime != nullptr) {
        JSRuntime *rt = mJsRuntime;
        JSContext *ctx = mJSContext;

        // 可能存在 Js 中 Export 类型的情况, 则需要在此处先进行反注册, jsState 释放会在 jsStateObjectFinalizer 中完成.
        // 异步 Worker 不被允许 Export 所以无需担心.
        GAnyToQJS::releaseJS(mJSState);

        JS_RunGC(mJsRuntime);

        js_std_free_handlers(rt);

        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);

        mJsRuntime = nullptr;
        mJSContext = nullptr;
        mJSState = nullptr;
    }
}
