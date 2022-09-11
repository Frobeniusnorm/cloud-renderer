#version 430

in vec3 linspace;
out vec4 color;

void main(){
  color = vec4(linspace, 1.0);
}
