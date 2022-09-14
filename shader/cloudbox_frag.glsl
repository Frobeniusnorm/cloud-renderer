#version 430
in vec3 linspace;
uniform vec3 eye;
uniform float stepSize;
uniform int backside;
uniform sampler2D backside_tex;
out vec4 color;

const vec3 sun_dir = normalize(vec3(0, 1, 0));
const vec3 domain_border_min = vec3(-1, -1, -1);
const vec3 domain_border_max = vec3(1,1,2);
vec2 hash( vec2 p ) // replace this by something better
{
	p = vec2( dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)) );
	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float noise( in vec2 p )
{
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;

	vec2  i = floor( p + (p.x+p.y)*K1 );
    vec2  a = p - i + (i.x+i.y)*K2;
    float m = step(a.y,a.x); 
    vec2  o = vec2(m,1.0-m);
    vec2  b = a - o + K2;
	vec2  c = a - 1.0 + 2.0*K2;
    vec3  h = max( 0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
	vec3  n = h*h*h*h*vec3( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));
    return dot( n, vec3(70.0) );
}
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
float testFunc(vec3 coord){
  coord *= 3.0;
  return clamp(noise(vec2(coord.x * 0.5, coord.z *0.5)) * (1.0 - coord.y * coord.y) - abs(coord.y*coord.y), 0.0, 1.0);
  
  

}
//marches a ray to the sun to calculate how much light is hitting the point
float transmittanceRay(vec3 start, float density){
  const int shadowSteps = 10;
  float res = 1.0; 
  for(int i = 0; i < shadowSteps; i++){
    vec3 pos = start + (i+1) * stepSize * sun_dir * 1.5;
    if(pos.y >= domain_border_max.y || pos.y <= domain_border_min.y) break; 
    float samp = testFunc(pos) * density;
    res *= (1.0 - samp);
  }
  return res;
}
vec4 raymarching(vec3 start, vec3 dir, vec3 end){
  const vec3 matcol = vec3(1);
  const float stepPerc = stepSize / length(end-start);
  const float density = stepSize * 30;
  float transmittance = 1.0;
  vec3 final = vec3(0);
  for(float curr = 0.0; curr <= 1.0; curr += stepPerc){
    vec3 samp = start + curr * dir * length(end-start);
    float samp_dens = testFunc(samp) * density;
    if(samp_dens > 0.0){
      //hit now attenuate
      float diffuse_co = transmittanceRay(samp, density) + 0.1;
      final += matcol * diffuse_co * transmittance * samp_dens;
      transmittance = (transmittance * (1.0 - samp_dens));
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
