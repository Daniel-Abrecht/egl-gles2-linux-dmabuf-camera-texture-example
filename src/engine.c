#include <time.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <engine.h>
#include <internal/engine.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#undef engine_load_create_shader_program


struct engine_display_driver* display_driver_list;


EGLImageKHR eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list) __attribute__((weak)); // May not be in libEGL symbol table, resolve manually :(
EGLImageKHR eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list){
  static PFNEGLCREATEIMAGEKHRPROC createImageProc = 0;
  if(!createImageProc)
    createImageProc = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
  return createImageProc(dpy, ctx, target, buffer, attrib_list);
}

EGLBoolean eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image) __attribute__((weak)); // May not be in libEGL symbol table, resolve manually :(
EGLBoolean eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image){
  static PFNEGLDESTROYIMAGEKHRPROC destroyImageProc = 0;
  if(!destroyImageProc)
    destroyImageProc = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
  return destroyImageProc(dpy, image);
}

void glDebugMessageCallbackKHR(GLDEBUGPROCKHR callback, const void *userParam) __attribute__((weak)); // May not be in libEGL symbol table, resolve manually :(
void glDebugMessageCallbackKHR(GLDEBUGPROCKHR callback, const void *userParam){
  static PFNGLDEBUGMESSAGECALLBACKKHRPROC debugMessageCallbackProc = 0;
  if(!debugMessageCallbackProc)
    debugMessageCallbackProc = (PFNGLDEBUGMESSAGECALLBACKKHRPROC)eglGetProcAddress("glDebugMessageCallbackKHR");
  debugMessageCallbackProc(callback, userParam);
}

void glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image) __attribute__((weak)); // May not be in libEGL symbol table, resolve manually :(
void glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image){
  static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC imageTargetTexture2DOES = 0;
  if(!imageTargetTexture2DOES)
    imageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
  imageTargetTexture2DOES(target, image);
}

void engine_i_register_display_driver(struct engine_display_driver* driver){
  driver->next = display_driver_list;
  display_driver_list = driver;
}

int engine_load_create_shader_program(struct shader* shader, struct engine_load_create_shader_program_params params){
  if(params.fragment_shader){
    shader->fragment = engine_load_shader("shader/test.fs");
    if(!shader->fragment){
      fprintf(stderr, "engine_load_shader failed\n");
      return -1;
    }
  }

  if(params.vertex_shader){
    shader->vertex = engine_load_shader("shader/test.vs");
    if(!shader->vertex){
      fprintf(stderr, "engine_load_shader failed\n");
      return -1;
    }
  }

  shader->program = engine_create_shader_program((GLuint[]){
    shader->vertex,
    shader->fragment,
  0});
  if(!shader->program){
    fprintf(stderr,"engine_create_shader_program failed\n");
    return -1;
  }

  return 0;
}

static void print_info_log(GLuint shader, bool shader_or_program){
  GLint length;
  if(shader_or_program){
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
  }else{
    glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &length);
  }
  if(!length)
    return;
  char* log = malloc(length+1);
  if(!log) return;
  if(shader_or_program){
    glGetShaderInfoLog(shader, length+1, 0, log);
  }else{
    glGetProgramInfoLog(shader, length+1, 0, log);
  }
  log[length] = 0;
  fputs(log , stderr);
  fputs("\n", stderr);
  free(log);
}

GLuint engine_load_shader(const char* path){
  size_t path_length = path ? strlen(path) : 0;
  if(path_length < 3){
    fprintf(stderr, "engine_load_shader called with invalid path\n");
    return 0;
  }
  GLenum type = 0;
  if(!strcmp(path+path_length-3,".vs")){
    type = GL_VERTEX_SHADER;
  }else if(!strcmp(path+path_length-3,".fs")){
    type = GL_FRAGMENT_SHADER;
  }else{
    fprintf(stderr, "Shader file has unknown extension. Currently supported are:  .vs - vertex shader,  .fs - fragment shader\n");
    return 0;
  }

  void* mem = 0;
  int fd = open(path, O_RDONLY);
  if(fd == -1){
    fprintf(stderr,"Failed to open file %s: %s\n",path,strerror(errno));
    return 0;
  }

  struct stat s;
  if(fstat(fd, &s) != 0){
    fprintf(stderr,"Failed to fstat file %s: %s\n",path,strerror(errno));
    close(fd);
    return 0;
  }

  if(!s.st_size){
    fprintf(stderr,"Shader file appears to be empty\n");
    close(fd);
    return 0;
  }

  mem = mmap(0, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if(mem == MAP_FAILED){
    fprintf(stderr, "mmap %s failed: %s\n", path, strerror(errno));
    close(fd);
    return 0;
  }

  close(fd);

  while(glGetError() != GL_NO_ERROR); // Clear previouse errors

  GLuint shader = glCreateShader(type);
  if(!shader){
    fprintf(stderr, "glCreateShader failed\n");
    munmap(mem, s.st_size);
    return 0;
  }

  glShaderSource(shader, 1, &(const GLchar*){mem}, &(GLint){s.st_size});
  glCompileShader(shader);
  print_info_log(shader, true);

  munmap(mem, s.st_size);

  GLint result = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
  if(result != GL_TRUE){
    fprintf(stderr,"Compiling shader failed\n");
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

GLuint engine_create_shader_program(GLuint shaders[]){
  GLuint shader_program = glCreateProgram();

  if(!shader_program){
    fprintf(stderr,"glCreateProgram failed\n");
    return 0;
  }

  for(GLuint* it=shaders; *it; it++)
    glAttachShader(shader_program, *it);

  glLinkProgram(shader_program);
  print_info_log(shader_program, false);

  GLint result = 0;
  glGetProgramiv(shader_program, GL_LINK_STATUS, &result);
  if(result != GL_TRUE){
    fprintf(stderr,"linking shader program failed\n");
    glDeleteProgram(shader_program);
    return 0;
  }

  return shader_program;
}


static int device_init_get_dmabuf(int fd, struct v4l2_format* fmt){

  {
    struct v4l2_capability cap;
    memset(&cap, 0, sizeof(cap));
    if( ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1){
      if(EINVAL == errno){
        fprintf(stderr, "This isn't a V4L2 device\n");
      }else{
        perror("VIDIOC_QUERYCAP");
      }
      return -1;
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
      fprintf(stderr, "This is no video capture device\n");
      return -1;
    }

    if(!(cap.capabilities & V4L2_CAP_STREAMING)){
      fprintf(stderr, "no streaming i/o support\n");
      return -1;
    }

  }

  {
    struct v4l2_cropcap cropcap;
    memset(&cropcap, 0, sizeof(cropcap));
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd, VIDIOC_CROPCAP, &cropcap) == 0){
      struct v4l2_crop crop;
      memset(&crop, 0, sizeof(crop));
      crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      crop.c = cropcap.defrect; /* reset to default */
      ioctl(fd, VIDIOC_S_CROP, &crop);
    }
  }

  memset(fmt,0,sizeof(*fmt));
  fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if(ioctl(fd, VIDIOC_G_FMT, fmt) == -1){
    perror("VIDIOC_G_FMT failed (now trying VIDIOC_S_FMT)");
    return -1;
  }

  unsigned int min = fmt->fmt.pix.width * 2;
  if (fmt->fmt.pix.bytesperline < min)
    fmt->fmt.pix.bytesperline = min;
  min = fmt->fmt.pix.bytesperline * fmt->fmt.pix.height;
  if (fmt->fmt.pix.sizeimage < min)
    fmt->fmt.pix.sizeimage = min;

  {
    struct v4l2_requestbuffers reqbuf;
    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    reqbuf.count = 1;
    if(ioctl(fd, VIDIOC_REQBUFS, &reqbuf) == -1){
      if(errno == EINVAL){
        fprintf(stderr, "Video capturing or DMABUF streaming is not supported\n");
      }else{
        perror("VIDIOC_REQBUFS");
      }
      return -1;
    }
  }

  struct v4l2_exportbuffer expbuf;
  memset(&expbuf, 0, sizeof(expbuf));
  expbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  expbuf.index = 0;
  expbuf.flags = O_RDONLY;
  if(ioctl(fd, VIDIOC_EXPBUF, &expbuf) == -1){
    perror("VIDIOC_EXPBUF");
    return -1;
  }

  return expbuf.fd;
}

static int start_capturing(int fd){
  if(ioctl(fd, VIDIOC_STREAMON, &(enum v4l2_buf_type){V4L2_BUF_TYPE_VIDEO_CAPTURE})){
    perror("VIDIOC_STREAMON");
    return -1;
  }
  return 0;
}

void engine_dma_texture_play(struct dma_gl_texture* texture){
  texture->autoupdate = true;
}

void engine_dma_texture_pause(struct dma_gl_texture* texture){
  texture->autoupdate = false;
}

GLenum engine_dma_texture_get_gl_type(struct dma_gl_texture* dgt){
  (void)dgt;
  return GL_TEXTURE_EXTERNAL_OES;
}

GLuint engine_dma_texture_get_gl_texture(struct dma_gl_texture* dgt){
  return dgt->texture;
}

int engine_dma_texture_update(struct dma_gl_texture* dgt){
  if(dgt->update_callback)
    return dgt->update_callback(dgt);
  return 0;
}

static int init(struct engine* engine, int argc, char* argv[]){
  (void)argc;
  (void)argv;
  memset(engine, 0, sizeof(*engine));
  for(struct engine_display_driver* it=display_driver_list; it; it=it->next){
    if(it->init && it->init(engine) != -1){
      engine->driver = it;
      break;
    }
  }
  if(!engine->driver){
    fprintf(stderr,"failed to initialise any display driver");
    return -1;
  }
  return 0;
}

// TODO: generalise format & make public
static struct dma_gl_texture* engine_dma_texture_create(struct engine* engine, int dma, struct v4l2_format* fmt){
  struct dma_gl_texture* dgt = calloc(1, sizeof(struct dma_gl_texture));
  if(!dgt){
    perror("calloc failed");
    goto error;
  }
  dgt->autoupdate = true;
  dgt->image = eglCreateImageKHR(engine->display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)0, (EGLint[]){
    EGL_WIDTH, fmt->fmt.pix.width,
    EGL_HEIGHT, fmt->fmt.pix.height,
    EGL_LINUX_DRM_FOURCC_EXT, fmt->fmt.pix.pixelformat,
    EGL_DMA_BUF_PLANE0_FD_EXT, dma,
    EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0, // No bound checks in drm intel driver in kernel (4.14.90) !?!
    EGL_DMA_BUF_PLANE0_PITCH_EXT, fmt->fmt.pix.bytesperline,
    EGL_NONE
  });
  if( dgt->image == EGL_NO_IMAGE_KHR ){
    fprintf(stderr,"eglCreateImageKHR failed\n");
    goto error_after_calloc;
  }
  while(glGetError() != GL_NO_ERROR); // Clear error flags
  glGenTextures(1, &dgt->texture);
  if(glGetError() != GL_NO_ERROR)
    goto error_after_create_image;
  glEnable(GL_TEXTURE_EXTERNAL_OES);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, dgt->texture);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
/*  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);*/
  glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, dgt->image);
  if(glGetError() != GL_NO_ERROR){
    fprintf(stderr,"creating gl texture failed\n");
    goto error_after_gen_textures;
  }
  dgt->engine = engine;
  dgt->next = engine->textures;
  dgt->last = 0;
  engine->textures = dgt;
  return dgt;
error_after_gen_textures:
  glDeleteTextures(1, &dgt->texture);
error_after_create_image:
  eglDestroyImageKHR(engine->display, dgt->image);
error_after_calloc:
  free(dgt);
error:
  return 0;
}

void engine_dma_texture_destroy(struct dma_gl_texture* dgt){
  if(!dgt)
    return;
  if(dgt->next)
    dgt->next->last = dgt->last;
  if(!dgt->last)
    dgt->engine->textures = dgt->next;
  if(dgt->last)
    dgt->last->next = dgt->next;
  if(dgt->destroy_callback)
    dgt->destroy_callback(dgt);
  glDeleteTextures(1, &dgt->texture);
  eglDestroyImageKHR(dgt->engine->display, dgt->image);
  memset(dgt, 0, sizeof(*dgt));
  free(dgt);
}

static int open_device(const char* dev){

  int fd = -1;
  if(strncmp(dev,"fd:",3) == 0){
    if(sscanf(dev+3,"%d",&fd) != 1){
      fprintf(stderr, "Cannot parse fd '%s'\n", dev);
      return -1;
    }
  }else{
     fd = open(dev, O_RDWR | O_NONBLOCK);
    if(fd == -1){
      fprintf(stderr, "Cannot open '%s': %d, %s\n", dev, errno, strerror(errno));
      return -1;
    }
  }

  struct stat st;
  if(fstat(fd, &st) == -1){
    fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev, errno, strerror(errno));
    return -1;
  }

  if(!S_ISCHR(st.st_mode)){
    fprintf(stderr, "%s isn't a device file\n", dev);
    return -1;
  }

  return fd;
}

void v4l_dma_destroy(struct dma_gl_texture* dgt){
  close(dgt->update_param.vlong);
}

int v4l_dma_update(struct dma_gl_texture* dgt){
  int ret = 0;
  int dev = dgt->update_param.vlong;

  struct v4l2_buffer buf;
  memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  buf.index = 0;

  if(ioctl(dev, VIDIOC_QUERYBUF, &buf) == -1){
    perror("VIDIOC_QUERYBUF");
    return -1;
  }

  if(buf.flags & V4L2_BUF_FLAG_DONE){
    if(ioctl(dev, VIDIOC_DQBUF, &buf) == -1){
      perror("VIDIOC_QBUF");
      return -1;
    }
    ret = 1;
  }

  if(!(buf.flags & V4L2_BUF_FLAG_QUEUED)){
    if(ioctl(dev, VIDIOC_QBUF, &buf) == -1){
      perror("VIDIOC_QBUF");
      return -1;
    }
  }

  return ret;
}

struct dma_gl_texture* engine_v4l_texture_create(struct engine* engine, const char* v4l_device){
  struct dma_gl_texture* result = 0;
  struct v4l2_format fmt;
  int dev = -1;
  int dma = -1;

  dev = open_device(v4l_device);
  if(dev == -1){
    fprintf(stderr,"failed to open v4l device\n");
    goto error;
  }

  dma = device_init_get_dmabuf(dev, &fmt);
  if(dma == -1){
    fprintf(stderr,"device_init_get_dmabuf failed\n");
    goto error;
  }

  if(start_capturing(dev) == -1){
    fprintf(stderr,"failed to start video capturing\n");
    goto error;
  }

  result = engine_dma_texture_create(engine, dma, &fmt);
  if(!result){
    fprintf(stderr,"failed to create texture from dma buffer\n");
    goto error;
  }

  result->update_callback = v4l_dma_update;
  result->destroy_callback = v4l_dma_destroy;
  result->update_param.vlong = dev;

  close(dma);

  return result;

error:
  close(dma);
  close(dev);
  engine_dma_texture_destroy(result);
  return 0;
}

void main_loop(struct engine* engine){
  while(true){
    eglMakeCurrent(engine->display, engine->surface, engine->surface, engine->context);
    if(engine->driver->before_drawing)
      engine->driver->before_drawing(engine);
    for(struct dma_gl_texture* it=engine->textures; it; it=it->next)
      if(it->autoupdate)
        engine_dma_texture_update(it);
    if(!engine_main_loop(engine))
      break;
    if(engine->driver->after_drawing)
      engine->driver->after_drawing(engine);
    eglSwapBuffers(engine->display, engine->surface);
  }
}

void cleanup(struct engine* engine){
  while(engine->textures)
    engine_dma_texture_destroy(engine->textures);
  if(engine->driver->destroy)
    engine->driver->destroy(engine);
  eglDestroyContext(engine->display, engine->context);
  eglDestroySurface(engine->display, engine->surface);
  eglTerminate(engine->display);
}

void engine_private_set(struct engine* engine, void* x){
  engine->private = x;
}

void* engine_private_get(struct engine* engine){
  return engine->private;
}

void gl_debug_callback(
  GLenum source,
  GLenum type,
  GLuint id,
  GLenum severity,
  GLsizei length,
  const GLchar* message,
  const void* userParam
){
  (void)source;
  (void)id;
  (void)length;
  (void)userParam;
  fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR_KHR ? "** GL ERROR **" : "" ),
            type, severity, message );
}

int main(int argc, char* argv[]){
  struct engine engine;
  if(init(&engine, argc, argv) == -1)
    goto error;
  eglMakeCurrent(engine.display, engine.surface, engine.surface, engine.context);
  glEnable(GL_DEBUG_OUTPUT_KHR);
  glDebugMessageCallbackKHR(gl_debug_callback, 0);
  if(engine_init(&engine, argc, argv) == -1)
    goto error_after_init;
  main_loop(&engine);
  eglMakeCurrent(engine.display, engine.surface, engine.surface, engine.context);
  engine_cleanup(&engine);
  cleanup(&engine);
  return 0;
error_after_init:
  cleanup(&engine);
error:
  return -1;
}
