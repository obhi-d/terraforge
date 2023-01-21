


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

vec2 random2(vec2 n) {
  return vec2(fract(sin(dot(n, vec2(12.9898, 78.233))) * 43758.5453));
}

float noise(vec2 st) {
  vec2 i = floor(st);
  vec2 f = fract(st);

  vec2 u = f * f * (3.0 - 2.0 * f);

  return mix(mix(dot(random2(i + vec2(0.0, 0.0)), f - vec2(0.0, 0.0)),
                dot(random2(i + vec2(1.0, 0.0)), f - vec2(1.0, 0.0)), u.x),
              mix(dot(random2(i + vec2(0.0, 1.0)), f - vec2(0.0, 1.0)),
                dot(random2(i + vec2(1.0, 1.0)), f - vec2(1.0, 1.0)), u.x), u.y);
}

vec3 get_foam(vec2 uv, float depth, float time) {
    // foam intensity
  float foam = smoothstep(0.08, 0.01, depth);
  foam = foam - (noise(uv + vec2(time * 0.1, 0.0)) + noise(uv + vec2(0.0, time * 0.1))) * 0.01;
  foam = clamp(foam, 0.0, 1.0) * 0.6;
  vec3 foam_color = vec3(1, 1, 1) * foam;
  return foam_color;
}

vec3 get_waves(vec2 uv, float depth, float time) {
    // Waves
  float wave_intensity = pow(abs(noise(vec2(uv.x + time * 0.1, uv.y + depth * 4.0 + time * 0.1))), 3.0) * .001;
  vec3 wave_color = vec3(0.5, 0.5, 1) * wave_intensity;
  return wave_color;
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
    vec3 bubble = get_waves(uv, water_depth, ftime);
    vec3 foam = get_foam(uv, water_depth, ftime);
    color_buffer = vec4(water_color.xyz + bubble + foam, 1.0);
  }
  else
  {
    color_buffer  = shadow_contrib * max(dot(light_dir, normal), 0.01) * sun_data.w * .01 * color;
  }
}

#endif