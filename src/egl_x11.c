#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <internal/engine.h>

struct xdisplay {
  Window window;
  Display* display;
};

static int init(struct engine* engine){
  /* Allocating data structures */
  struct xdisplay* xd = calloc(1, sizeof(struct xdisplay));
  if(!xd){
    perror("calloc failed");
    return -1;
  }
  engine->driver_private = xd;

  /* Create window */
  xd->display = XOpenDisplay(0);
  if(!xd->display){
    fprintf(stderr, "Cannot open display\n");
    return -1;
  }
  int screen = DefaultScreen(xd->display);
  xd->window = XCreateSimpleWindow(xd->display, RootWindow(xd->display, screen), 50, 50, 800, 600, true, BlackPixel(xd->display, screen), BlackPixel(xd->display, screen));
  XSelectInput(xd->display, xd->window, ExposureMask | KeyPressMask);
  XMapWindow(xd->display, xd->window);

  /* initialise egl context */

  engine->display = eglGetDisplay((EGLNativeDisplayType)xd->display);
  if( engine->display == EGL_NO_DISPLAY ){
    fprintf(stderr, "Got no EGL display.");
    return -1;
  }

  if(!eglInitialize(engine->display, 0, 0)){
    fprintf(stderr, "Unable to initialize EGL");
    return -1;
  }

  EGLint attr[] = {
    EGL_BUFFER_SIZE, 16,
    EGL_RENDERABLE_TYPE,
    EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

  EGLint num_config;
  if( !eglChooseConfig(engine->display, attr, &engine->config, 1, &num_config) ){
    fprintf(stderr, "Failed to choose config (eglError: %d)\n", eglGetError());
    return -1;
  }

  if(num_config != 1) {
    fprintf(stderr, "Didn't get exactly one config, but %d\n", num_config);
    return -1;
  }

  engine->surface = eglCreateWindowSurface(engine->display, engine->config, xd->window, 0);
  if( engine->surface == EGL_NO_SURFACE ){
    fprintf(stderr, "Unable to create EGL surface (eglError: %d)\n", eglGetError());
    return -1;
  }

  EGLint ctxattr[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };
  engine->context = eglCreateContext(engine->display, engine->config, EGL_NO_CONTEXT, ctxattr);
  if( engine->context == EGL_NO_CONTEXT ){
    fprintf(stderr, "Unable to create EGL context (eglError: %d)\n", eglGetError());
    return -1;
  }

  return 0;
}

void destroy(struct engine* engine){
  struct xdisplay* xd = engine->driver_private;
  XDestroyWindow(xd->display, xd->window);
  XCloseDisplay(xd->display);
}

void before_drawing(struct engine* engine){
  struct xdisplay* xd = engine->driver_private;
//  XEvent event;
//  XNextEvent(xd->display, &event); // includes keyboard input
  XWindowAttributes gwa;
  XGetWindowAttributes(xd->display, xd->window, &gwa);
  glViewport(0, 0, gwa.width, gwa.height);
  // TODO
}

static struct engine_display_driver display_driver = {
  .name = "X11",
  .init = init,
  .destroy = destroy,
  .before_drawing = before_drawing
};
ENGINE_REGISTER_DISPLAY_DRIVER(&display_driver)
