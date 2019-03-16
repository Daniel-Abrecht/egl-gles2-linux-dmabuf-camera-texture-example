#include <engine.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <GLES2/gl2.h>

struct my_test_shader {
  struct shader shader;
  struct {
    GLint position;
    GLint texture_coordinate;
  } attribut;
  struct {
    GLint texture;
  } uniform;
};

struct runtime {
  struct dma_gl_texture* camera;
  struct my_test_shader test_shader;
};

int engine_init(struct engine* engine, int argc, char* argv[]){
  (void)argc;
  (void)argv;

  /* Allocate some private date to store everything in */
  struct runtime* runtime = calloc(1, sizeof(struct runtime));
  if(!runtime){
    perror("calloc failed");
    goto error;
  }
  engine_private_set(engine, runtime);

  /* Load & compile shaders */
  if(engine_load_create_shader_program(
    &runtime->test_shader.shader,
    .vertex_shader = "shader/test.vs",
    .fragment_shader = "shader/test.fs"
  ) == -1){
    fprintf(stderr, "engine_load_create_shader_program failed\n");
    goto error_after_calloc;
  }

  /* get shader attributes & uniforms */
  runtime->test_shader.attribut.position = glGetAttribLocation(runtime->test_shader.shader.program, "position");
  if(runtime->test_shader.attribut.position == -1){
    fprintf(stderr,"position attribute not found in shader program\n");
    goto error_after_calloc;
  }

  runtime->test_shader.attribut.texture_coordinate = glGetAttribLocation(runtime->test_shader.shader.program, "texture_coordinate");
  if(runtime->test_shader.attribut.texture_coordinate == -1){
    fprintf(stderr,"texture_coordinate attribute not found in shader program\n");
    goto error_after_calloc;
  }

  runtime->test_shader.uniform.texture = glGetUniformLocation(runtime->test_shader.shader.program, "texture");
  if(runtime->test_shader.uniform.texture == -1){
    fprintf(stderr,"texture uniform not found in shader program\n");
    goto error_after_calloc;
  }

  /* Create texture from v4l device */
  runtime->camera = engine_v4l_texture_create(engine, "/dev/video0");
  if(!runtime->camera){
    fprintf(stderr, "engine_v4l_texture_create failed\n");
    goto error_after_calloc;
  }

  return 0;

error_after_calloc:
  free(runtime);
error:
  return -1;
}

bool engine_main_loop(struct engine* engine){
  struct runtime* runtime = engine_private_get(engine);

  glUseProgram(runtime->test_shader.shader.program);

  glClearColor(0.1,0.2,0.3,1);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  float position[][3][3] = {
    { {-0.5, -0.5, 0.0}, { 0.5, -0.5, 0.0}, {-0.5, 0.5, 0.0} },
    { {-0.5,  0.5, 0.0}, { 0.5, -0.5, 0.0}, { 0.5, 0.5, 0.0} }
  };
  size_t position_count = sizeof(position)/sizeof(**position);

  float texture_coordinate[][3][2] = {
    { { 1.0,  1.0}, { 0.0,  1.0}, { 1.0, 0.0} },
    { { 1.0,  0.0}, { 0.0,  1.0}, { 0.0, 0.0} }
  };
  size_t texture_coordinate_count = sizeof(texture_coordinate)/sizeof(**texture_coordinate);

  assert(texture_coordinate_count == position_count);
  size_t vertex_count = position_count;

  glVertexAttribPointer(runtime->test_shader.attribut.position, 3, GL_FLOAT, false, 0, position);
  glEnableVertexAttribArray(runtime->test_shader.attribut.position);
  glVertexAttribPointer(runtime->test_shader.attribut.texture_coordinate, 2, GL_FLOAT, false, 0, texture_coordinate);
  glEnableVertexAttribArray(runtime->test_shader.attribut.texture_coordinate);

  glActiveTexture(GL_TEXTURE0);
//  glEnable(GL_TEXTURE_2D);
//  glEnable(engine_dma_texture_get_gl_type(runtime->camera));
  glBindTexture(
    engine_dma_texture_get_gl_type(runtime->camera),
    engine_dma_texture_get_gl_texture(runtime->camera)
  );
  glUniform1i(runtime->test_shader.uniform.texture, 0); // GL_TEXTURE0

  glDrawArrays(GL_TRIANGLES, 0, vertex_count);

  return true;
}

void engine_cleanup(struct engine* engine){
  struct runtime* runtime = engine_private_get(engine);
  if(!runtime)
    return;
  // TODO
}
