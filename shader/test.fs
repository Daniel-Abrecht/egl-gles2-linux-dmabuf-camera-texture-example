#version 300 es
// Took me forever to figure out that the extension is requiered!!!
#extension GL_OES_EGL_image_external : require

precision mediump float;

uniform samplerExternalOES texture;

in vec2 f_texture_coordinate;

out vec4 color;

void main(){
  color = vec4(vec3(texture2D(texture, f_texture_coordinate)), 1.0);
}
