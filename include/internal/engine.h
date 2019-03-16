#ifndef DENG_I_ENGINE_H
#define DENG_I_ENGINE_H

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <stdbool.h>

#ifndef CONCAT
#define CONCAT(A,B) A ## B
#endif

#ifndef CONCAT_EVAL
#define CONCAT_EVAL(A,B) CONCAT(A,B)
#endif

#define ENGINE_REGISTER_DISPLAY_DRIVER(X) \
  static void CONCAT_EVAL(erdd_reg_,__LINE__)(void) __attribute__((constructor)); \
  static void CONCAT_EVAL(erdd_reg_,__LINE__)(void){ \
    engine_i_register_display_driver((X)); \
  }

struct engine {
  struct engine_display_driver* driver;
  void* driver_private;
  EGLDisplay display;
  EGLConfig config;
  EGLContext context;
  EGLSurface surface;
  struct dma_gl_texture* textures;
  void* private;
};

struct dma_gl_texture {
  struct engine* engine;
  GLuint texture;
  EGLImageKHR image;
  int (*update_callback)(struct dma_gl_texture*);
  void (*destroy_callback)(struct dma_gl_texture*);
  union {
    long vlong;
    void* vptr;
  } update_param;
  bool autoupdate;
  struct dma_gl_texture *next, *last;
};

struct engine_display_driver {
  const char* name;
  struct engine_display_driver* next;
  int(*init)(struct engine* engine);
  void(*before_drawing)(struct engine* engine);
  void(*after_drawing)(struct engine* engine);
  void(*destroy)(struct engine* engine);
};

void engine_i_register_display_driver(struct engine_display_driver* driver);
int engine_i_egl_x11_init(struct engine* engine);

#endif
