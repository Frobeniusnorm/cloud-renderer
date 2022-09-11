#version 430

in vec3 linspace;
uniform vec3 eye;
uniform int backside;
uniform sampler2D backside_tex;
out vec4 color;

void main(){
  if(backside == 1){
  color = vec4(linspace, 1.0);
  }else{
    vec2 tc = (gl_FragCoord.xy) / textureSize(backside_tex, 0);
  vec3 backside_pos = texture(backside_tex, tc).rgb;
  color = vec4(0, length(backside_pos - linspace),0, 1.0);
  }

}
