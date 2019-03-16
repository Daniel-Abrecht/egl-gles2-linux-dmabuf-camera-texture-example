#version 300 es

precision highp float;

in vec4 position;
in vec2 texture_coordinate;

out vec2 f_texture_coordinate;

void main(){
  f_texture_coordinate = texture_coordinate;
  gl_Position = position;
}
