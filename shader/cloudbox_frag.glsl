#version 430

in vec3 linspace;
uniform vec3 eye;
uniform int backside;
uniform sampler2D backside_tex;
out vec4 color;

const vec3 domain_border_min = vec3(-1, -1, -1);
const vec3 domain_border_max = vec3(1,1,2);

float onBorder(vec3 pos){
  const float border_size = 0.002 * length(eye - pos);
  int num_near_zero = 0;
  float att[3];
  int index = 0;
  for(int i = 0; i < 3; i++){
    if(pos[i] <= domain_border_min[i] + border_size){
      att[num_near_zero++] = 1.0 - ((pos[i] - domain_border_min[i]) / border_size);
    }
    if(pos[i] >= domain_border_max[i]-border_size){
      att[num_near_zero++] = (pos[i] - (domain_border_max[i] - border_size)) / border_size;
    }
  }
  float a = 1.0;
  if(num_near_zero > 1){
   for(int i = 0; i < num_near_zero; i++){
     //if weakest att dont multiply
     if(num_near_zero == 2 || (att[(i + 1)%3] < att[i] || att[(i+2)%3] < att[i])){
       a *= att[i];
     }
   }
   return clamp(a, 0.0, 1.0);
  }else return 0.0;
}

const vec3 skyColor = vec3(0.2, 0.2, 0.5);
bool testFunc(vec3 coord){
  return sin(coord.x * 10) * cos(coord.y * 5) + sin(coord.z * 3) > 0.5;
}

vec4 raymarching(vec3 start, vec3 dir, vec3 end){
  const vec3 matcol = vec3(.9);
  const float stepSize = 0.02;
  const float stepPerc = stepSize / length(end-start);
  const float density = stepSize * 4;
  float transmittance = 1.0;
  vec3 final = vec3(0);
  for(float curr = 0.0; curr <= 1.0; curr += stepPerc){
    vec3 samp = start + curr * dir * length(end-start);
    if(testFunc(samp)){
      //hit now attenuate
      final += matcol * transmittance * density;
      transmittance = (transmittance * (1.0 - density));
      if(transmittance < 0.05) break;
    }
  }
  return vec4(final, 1.0 - transmittance);
}

void main(){
  if(backside == 1){
  color = vec4(linspace, 1.0);
  }else{
    //raymarching
    float front_border = onBorder(linspace);

    vec2 tc = (gl_FragCoord.xy) / textureSize(backside_tex, 0);
    vec3 backside_pos = texture(backside_tex, tc).rgb;
    float back_border = onBorder(backside_pos);
    vec3 background = mix(skyColor, vec3(0.15), back_border);

    float total_length = length(backside_pos - linspace);
    vec3 dir = normalize(backside_pos - linspace);
    vec4 final = raymarching(linspace, dir, backside_pos);
    final.rgb += (1.0 - final.a) * background;
    color = vec4(mix(final.rgb, vec3(0.15), front_border), 1.0);
  }

}
