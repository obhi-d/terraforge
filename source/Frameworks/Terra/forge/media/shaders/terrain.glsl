


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

layout(location = 0) out highp vec3 world_pos;
layout(location = 1) out highp vec4 shadow_pos;
layout(location = 2) out highp vec2 uv;

void main()
{
  uint x = gl_VertexID % width;
  uint y = gl_VertexID / width;

  uv = vec2(float(x) * rwidth, float(y) * rheight);

  float z   = texture(heights, uv).x;
  world_pos = vec3(float(x) - float(width - 1) * 0.5, z, float(y) - float(height - 1) * 0.5);

  vec4 s = shadow_view_projection * vec4(world_pos, 1.0);
  shadow_pos = s;
  gl_Position = view_projection * vec4(world_pos, 1.0);
}

#elif defined(FragmentShader)

layout(location = 0) in highp vec3 world_pos;
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

void main()
{  
  
  vec2  tex_size       = vec2(1.0f / float(ShadowTextureDim));

  vec3  x              = dFdx(world_pos);
  vec3  y              = dFdy(world_pos);

  vec3 normal = normalize(cross(x, y));

  vec3 light_dir = sun_data.xyz;

  // float bias = max(ShadowBiasMax * (1.0 - dot(normal, light_dir)), ShadowBiasMin);
  // vec3  shadow_lookup = shadow_pos.xyz;
  // shadow_lookup.z += bias;
  //shadow_lookup.z += ShadowBiasMin;
  //float shadow_contrib = 1.0;
  //if ( texture( shadow_map, shadow_pos.xy ).z  > (shadow_pos.z + bias) )
  //{
  //  shadow_contrib = 0.5;
  //}
  float shadow_contrib = 0.0;
  for(int i = -1; i < 2; i++)
    for(int j = -1; j < 2; j++)
      shadow_contrib += 0.11 * textureProjOffset(shadow_map, shadow_pos, ivec2(i, j));


  float water_contrib      = texture(water, uv).x;
  float vegetation_contrib = texture(vegetation, uv).x;
  float rocks_contrib      = texture(rocks, uv).x;
  // layer_contrib.x : water
  // layer_contrib.y : grass/vegetation
  // layer_contrib.z : rocks
  // layer_contrib.w : default color
  vec4 weights = layer_weights;
  vec4 color = (texture(layer_colors, vec2(water_contrib, 0.0))  * weights.x +
               texture(layer_colors, vec2(vegetation_contrib, 1.0)) * weights.y +
               texture(layer_colors, vec2(rocks_contrib, 2.0))  * weights.z +
               texture(layer_colors, vec2((world_pos.y - hrange.x) * hrange.y, 3.0))  * weights.w) / 
               (weights.x + weights.y + weights.z + weights.w);

  color_buffer  = shadow_contrib * max(dot(light_dir, normal), 0.01) * sun_data.w * .01 * color;
}

#endif