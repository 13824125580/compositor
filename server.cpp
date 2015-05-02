
#include "compositor.hpp"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <cassert>

#include <X11/Xlib.h>

static Display* xdisp;
static Window window;

EGLDisplay eglDisplay;
EGLContext eglContext;
EGLSurface eglSurface;

#ifndef EGL_PLATFORM_X11_KHR
#define EGL_PLATFORM_X11_KHR 0x31D5
#endif

static void initX()
{
    xdisp = XOpenDisplay( NULL );
    if( xdisp == NULL ) {
        printf("!!! XOpenDisplay\n");
        exit(1);
    }
 
    Window rootWin = DefaultRootWindow( xdisp );
 
    int screen = DefaultScreen( xdisp );
    unsigned long white = WhitePixel( xdisp, screen );
    unsigned long black = BlackPixel( xdisp, screen );
 
    window = XCreateSimpleWindow( xdisp, rootWin,
                                  100, 100,
                                  800, 600,
                                  2,
                                  black, white );
 
 
    XSelectInput( xdisp, window, KeyPressMask );
 
    XMapWindow( xdisp, window );

    XFlush(xdisp);
}

void egl_init(wl_display* wl_dpy)
{
    EGLConfig config = 0;
    EGLint major, minor;
    EGLint num;

    initX();

    PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display =
        reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));

    eglDisplay = get_platform_display(EGL_PLATFORM_X11_KHR,
                                      (void*)xdisp,
                                      NULL);

//    eglDisplay = eglGetDisplay((EGLNativeDisplayType)window);
    if(!eglInitialize(eglDisplay, &major, &minor)){
        printf("!!! eglInitialize\n");
        exit(1);
    }

    EGLint attr[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 0,
        EGL_BUFFER_SIZE, 32,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    eglChooseConfig(eglDisplay, attr, &config, 1, &num);
    printf("num: %d\n", num);

    EGLint ctxattr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE,
    };

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        fprintf( stderr, "failed set EGL_OPENGL_ES_API");
        exit(1);
    }

    eglContext = eglCreateContext( eglDisplay, config, EGL_NO_CONTEXT, ctxattr );
    if( eglContext == EGL_NO_CONTEXT ) {
        fprintf( stderr, "Unable to create EGL context. (%X)\n", eglGetError() );
        return;
    }

    eglSurface = eglCreateWindowSurface(eglDisplay, config, window, NULL);

    eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);

    PFNEGLBINDWAYLANDDISPLAYWL bind_display =
        reinterpret_cast<PFNEGLBINDWAYLANDDISPLAYWL>(eglGetProcAddress("eglBindWaylandDisplayWL"));

    if(bind_display) {
        bind_display(eglDisplay, wl_dpy);
    }

}

int main(void) {

    wl_display* dpy = wl_display_create();

    egl_init(dpy);

    yawc::compositor::global_create(dpy);

    const int stat = wl_display_add_socket(dpy, "wayland-0");
    assert(stat == 0);

    wl_display_run(dpy);

    return 0;
}

