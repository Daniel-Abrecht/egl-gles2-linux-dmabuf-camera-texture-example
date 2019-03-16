#ifndef DENG_ENGINE_H
#define DENG_ENGINE_H

#include <stdbool.h>
#include <GLES2/gl2.h>

struct engine;
struct dma_gl_texture;

struct shader {
  GLuint fragment;
  GLuint vertex;
  GLuint program;
};

struct engine_load_create_shader_program_params {
  const char* fragment_shader;
  const char* vertex_shader;
};

int engine_init(struct engine* engine, int argc, char* argv[]);
bool engine_main_loop(struct engine* engine);
void engine_cleanup(struct engine* engine);

void engine_private_set(struct engine* engine, void* x);
void* engine_private_get(struct engine* engine);

GLuint engine_load_shader(const char* path);
GLuint engine_create_shader_program(GLuint shaders[]);
int engine_load_create_shader_program(struct shader* result, struct engine_load_create_shader_program_params); // Convinience function
#define engine_load_create_shader_program(X,...) engine_load_create_shader_program(X,(struct engine_load_create_shader_program_params){__VA_ARGS__})

struct dma_gl_texture* engine_v4l_texture_create(struct engine* engine, const char* v4l_device);
void engine_dma_texture_destroy(struct dma_gl_texture* dgt);
GLuint engine_dma_texture_get_gl_texture(struct dma_gl_texture* dgt);
GLenum engine_dma_texture_get_gl_type(struct dma_gl_texture* dgt);
void engine_dma_texture_bind(struct dma_gl_texture* dgt);
void engine_dma_texture_play(struct dma_gl_texture* texture);
void engine_dma_texture_pause(struct dma_gl_texture* texture);
int engine_dma_texture_update(struct dma_gl_texture* texture);

#endif
