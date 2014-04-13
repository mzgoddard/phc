#ifndef OPENGL_H_QOVB9FTT
#define OPENGL_H_QOVB9FTT

#ifdef __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_OS_IPHONE
        #include <OpenGLES/ES2/gl.h>
    #else // Mac
        #include <OpenGL/gl.h>
    #endif
#else
    #if EMSCRIPTEN
        #include <GLES2/gl2.h>
        #include <EGL/egl.h>
    #else
        #if ANDROID
            #include <GLES2/gl2.h>
            #include <GLES2/gl2ext.h>
        #else
            #include <GL/gl.h>
        #endif
    #endif
#endif

#endif /* end of include guard: OPENGL_H_QOVB9FTT */
