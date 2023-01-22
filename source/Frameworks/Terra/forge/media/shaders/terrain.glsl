


// uint width;
// uint height;
// float rwidth;
// float rheight;
// float height_multiplier
// vec4 sun_data;
// vec4 layer_weights;
// mat4 shadow_view_projection
// mat4 view_projection
// sampler1DArray layer_colors;
// sampler2D heights;
// sampler2DShadow shadow_map;
// sampler2D layers;

#if defined(VertexShader)

layout(location = 0) out highp vec4 world_pos;
layout(location = 1) out highp vec4 shadow_pos;
layout(location = 2) out highp vec2 uv;

float get_water_level()
{
  #ifdef HasWaterLevel
    return imageLoad(water_level, ivec2(gl_GlobalInvocationID.xy)).x;
  #else
    return water_level;
  #endif
}

void main()
{
  uint x = gl_VertexID % width;
  uint y = gl_VertexID / width;

  uv = vec2(float(x) * rwidth, float(y) * rheight);

  float z   = hscale * texelFetch(heights, ivec2(x, y), 0).x;

#ifdef ShowWaterLevel
  float water = hscale * get_water_level();
  float water_depth = water - z;
  vec3  wpos = vec3(float(x) - float(width - 1) * 0.5, water_depth > 0.0 ? water : z, float(y) - float(height - 1) * 0.5);
#else
  float water = z;
  const float water_depth = 0.0;
  vec3  wpos = vec3(float(x) - float(width - 1) * 0.5, z, float(y) - float(height - 1) * 0.5);
#endif

  vec4 s = shadow_view_projection * vec4(wpos, 1.0);
  shadow_pos = s;
  world_pos = vec4(wpos, max(water_depth, 0.0));
  gl_Position = view_projection * vec4(wpos, 1.0);
}

#elif defined(FragmentShader)

layout(location = 0) in highp vec4 world_pos;
layout(location = 1) in highp vec4 shadow_pos;
layout(location = 2) in highp vec2 uv;

#if defined(Enum_ShadowRes512)
#define ShadowTextureDim 512
#elif defined(Enum_ShadowRes1024)
#define ShadowTextureDim 1024
#elif defined(Enum_ShadowRes2048)
#define ShadowTextureDim 2048
#elif defined(Enum_ShadowRes4096)
#define ShadowTextureDim 4096
#endif

#define ShadowBiasMax 0.0005
#define ShadowBiasMin 0.00001

const float PI = 3.1415926535897932384626433832795;
const float waveSpeed = 0.1;
const float waveHeight = 0.1;
const float waveLength = 0.1;
const float bubbleSize = 0.005;
const float bubbleSpeed = 0.1;
const float foamSize = 0.01;
const float foamSpeed = 0.05;

// Author @patriciogv - 2015
// http://patriciogonzalezvivo.com
// Author @patriciogv - 2015
// http://patriciogonzalezvivo.com

vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }

float noise(vec2 v) {
    const vec4 C = vec4(0.211324865405187,  // (3.0-sqrt(3.0))/6.0
                      0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
                      -0.577350269189626,  // -1.0 + 2.0 * C.x
                      0.024390243902439); // 1.0 / 41.0
    vec2 i  = floor(v + dot(v, C.yy) );
    vec2 x0 = v -   i + dot(i, C.xx);
    vec2 i1;
    i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec4 x12 = x0.xyxy + C.xxzz;
    x12.xy -= i1;
    i = mod289(i); // Avoid truncation effects in permutation
    vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
                  + i.x + vec3(0.0, i1.x, 1.0 ));
    vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
    m = m*m ;
    m = m*m ;
    vec3 x = 2.0 * fract(p * C.www) - 1.0;
    vec3 h = abs(x) - 0.5;
    vec3 ox = floor(x + 0.5);
    vec3 a0 = x - ox;
    m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
    vec3 g;
    g.x  = a0.x  * x0.x  + h.x  * x0.y;
    g.yz = a0.yz * x12.xz + h.yz * x12.yw;
    return 130.0 * dot(m, g);
}

float compute_shore_color(in vec3 wpos, in vec3 light_dir, in vec2 uv, in float depth, in float time) 
{
  float depthoff = depth + sin(time);
  vec3 wh = vec3(world_pos.x, world_pos.y + depthoff, world_pos.z);
  vec3 x = dFdx(wh);
  vec3 y = dFdy(wh);
  vec3 normal  = normalize(cross(x, y));
  float specular = pow(max(dot(normal, light_dir), 0.0), 15.0);
  float fresnel =  pow(1.0 - max(dot(vec3(0.0, 1.0, 0.0), normalize(vec3(0.0, 1.0, 0.0) + normalize(vec3(uv, depth)))), 5.0);
  return fresnel + specular;
}

void main()
{  
  vec2  tex_size       = vec2(1.0f / float(ShadowTextureDim));

  vec3  x              = dFdx(world_pos.xyz);
  vec3  y              = dFdy(world_pos.xyz);

  vec3 normal  = normalize(cross(x, y));
  vec4 weights = layer_weights;

  vec3 light_dir = sun_data.xyz;
  
  float shadow_contrib = 0.0;
  for(int i = -1; i < 2; i++)
    for(int j = -1; j < 2; j++)
      shadow_contrib += 0.11 * textureProjOffset(shadow_map, shadow_pos, ivec2(i, j));

  float vegetation_contrib = texture(vegetation, uv).x;
  float rocks_contrib      = texture(rocks, uv).x;

  vec4 color = (
              texture(layer_colors, vec2(vegetation_contrib, 1.0)) * weights.y +
              texture(layer_colors, vec2(rocks_contrib, 2.0)) * weights.z +
              texture(layer_colors, vec2((world_pos.y - hrange.x) * hrange.y, 3.0))  * weights.w) / 
              (weights.y + weights.z + weights.w);

  // if we are in land
  if (world_pos.w > 0.0)
  {
    float water_depth = world_pos.w / hscale;
  
    vec4 water_color = texture(layer_colors, vec2(water_depth, 0.0))  * weights.x;
    color_buffer = water_color +  compute_shore_color(world_pos.xyz, light_dir, uv, water_depth, ftime);
 
  }
  else
  {
    color_buffer  = shadow_contrib * max(dot(light_dir, normal), 0.01) * sun_data.w * .01 * color;
  }
}

#endif