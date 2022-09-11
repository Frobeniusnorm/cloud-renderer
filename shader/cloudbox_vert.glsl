#version 430 
in vec3 coords;
out vec3 linspace;
uniform mat4 cammat;
void main(){
  gl_Position = cammat * vec4(coords, 1.0);
  linspace = coords * 0.5 + 0.5;
}
