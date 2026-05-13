//
// Created by Gxin on 25-5-22.
//

#ifndef QJSCONF_H
#define QJSCONF_H

/*
 * gx-script compatibility layer.
 *
 * quickjs-ng 0.14.0 moved symbol visibility handling into quickjs.h and uses
 * BUILDING_QJS_SHARED while building qjs, and USING_QJS_SHARED for consumers.
 * Keep this header so local integration points can continue to include it, but
 * map the old BUILD_SHARED_LIBS convention to the new upstream macros.
 */
#if defined(BUILD_SHARED_LIBS) && defined(QUICKJS_NG_BUILD)
#if defined(_WIN32) || defined(_WIN64)
#ifndef BUILDING_QJS_SHARED
#define BUILDING_QJS_SHARED
#endif
#endif
#endif

#endif //QJSCONF_H
