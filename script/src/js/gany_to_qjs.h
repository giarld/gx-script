//
// Created by Gxin on 25-5-23.
//

#ifndef GX_GANY_TO_QJS_H
#define GX_GANY_TO_QJS_H

#include <gx/gany.h>

#include "gany_js_impl_qjs.h"


class GAnyToQJS
{
public:
    static bool toJS(JS_State *jsState, bool isWorker);

    static bool releaseJS(JS_State *jsState);

public:
    static GAny makeJsValueToGAny(const JS_State *jsState, const JSValue &value);

    static JSValue makeGAnyToJsValue(const JS_State *jsState, const GAny &value, bool unwrap);

private:
    static void setGAnyGeneralProto(const JS_State *jsState, JSValue proto);

    static void JS_GAnyFinalizer(JSRuntime *rt, JSValue val);

    static JSValue JS_GAnyCreate(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyCreateWorkerCallable(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyImport(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyParseJson(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyClass(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyOp(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyBind(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);


    static JSValue JS_GAnyClone(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyClassTypeName(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyTypeName(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyType(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyLength(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnySize(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyDump(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyClassObject(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);


    static JSValue JS_GAnyIs(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyIsUndefined(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyIsNull(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyIsFunction(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyIsClass(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyIsException(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyIsObject(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyIsArray(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyIsInt(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyIsFloat(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyIsDouble(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyIsNumber(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyIsString(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyIsBoolean(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyIsUserObject(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);


    static JSValue JS_GAnyClassNew(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyToString(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyToInt32(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyToInt64(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyToFloat(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyToBool(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyToJsValue(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyToPrimitive(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyToJsonString(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyContains(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyErase(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyPushBack(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyClear(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnySetItem(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyGetItem(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyDelItem(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyIterator(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyHasNext(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyNext(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);


    static JSValue JS_GAnyCall(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv);

    static JSValue JS_GAnyCallMethod(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv, int magic, JSValue *funcData);

    static JSValue JS_GAnyGetterSetter(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv, int magic, JSValue *funcData);

private:
    static GAny getGAnyClassDB();

    static void makeObjectItemGetSet(JSContext *ctx, const JSValue &proto, const std::string &name, bool canGet, bool canSet);
};

#endif //GX_GANY_TO_QJS_H
