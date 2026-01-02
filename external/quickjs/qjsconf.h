//
// Created by Gxin on 25-5-22.
//

#ifndef QJSCONF_H
#define QJSCONF_H

#if defined(BUILD_SHARED_LIBS)
#if defined(_WIN32) || defined(_WIN64)

#define JS_EXTERN __declspec( dllexport )

#else

#define JS_EXTERN __attribute__((visibility("default")))

#endif
#else

#define JS_EXTERN extern

#endif

#endif //QJSCONF_H
