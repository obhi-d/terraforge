

// uint width;
// uint height;
// float rwidth;
// float rheight;
// float height_multiplier
// vec3 sun_direction;
// float sun_intensity;
// vec4 layer_weights;
// sampler1DArray layer_colors;
// sampler2D heights;
// sampler2DShadow shadow_map;
// sampler2D layers;
// mat4 shadow_view_projection
// mat4 view_projection
layout(location = 0) in highp vec3 world_pos;
layout(location = 1) in highp vec3 shadow_pos;
layout(location = 2) in highp vec2 uv;

layout(location = 0) out highp vec4 color_buffer;

#if defined(ShadowRes0)
#define ShadowTextureDim 512
#elif defined(ShadowRes1)
#define ShadowTextureDim 1024
#elif defined(ShadowRes2)
#define ShadowTextureDim 2048
#elif defined(ShadowRes3)
#define ShadowTextureDim 4096
#endif

#define ShadowBiasMax 0.05
#define ShadowBiasMin 0.0001

void main()
{
  sampler2DShadow shadow = shadow_map;
  sampler1DArray  colors = layer_colors;

  shadow_pos           = shadow_pos * 0.5 + 0.5;
  vec2  tex_size       = vec2(1.0f / (float)ShadowTextureDim);
  float shadow_contrib = 0.f;
  vec3  x              = dFdx(world_pos);
  vec3  y              = dFdy(world_pos);

  vec3 normal = normalize(cross(x, y));

  vec3 light_dir = -sun_dir;

  float bias = max(ShadowBiasMax * (1.0 - dot(normal, light_dir)), ShadowBias);
  for (int x = -1; x <= 1; ++x)
  {
    for (int y = -1; y <= 1; ++y)
    {
      shadow_contrib += texture(shadow, vec3(shadow_pos.xy + vec2(x, y) * tex_size, shadow_pos.z - bias));
      if (shadow_pos.z - bias > depth)
        shadow_contrib += (0.002 / 9.0);
      else
        shadow_contrib += (1.0 / 9.0);
    }
  }

  vec4 layer_contrib = texture(layers, uv);
  // layer_contrib.x : water
  // layer_contrib.y : rocks
  // layer_contrib.z : grass/vegetation
  // layer_contrib.w : default color
  vec4 weights = layer_weight * layer_contrib;
  vec4 color = (texture(layer_colors, vec2(layer_contrib.x, 0.0))  * weights.x +
               texture(layer_colors, vec2(layer_contrib.y, 0.25)) * weights.y +
               texture(layer_colors, vec2(layer_contrib.z, 2.0))  * weights.z +
               texture(layer_colors, vec2(layer_contrib.w, 3.0))  * weights.w) / 
               (weights.x + weights.y + weights.z + weights.w);

  color_buffer  = (1.0 - shadow_contrib) * max(dot(light_dir, normal), 0.01) * sun_intensity * color;
}