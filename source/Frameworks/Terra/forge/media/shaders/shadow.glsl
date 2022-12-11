

// uint width;
// uint height;
// float rwidth;
// float rheight;
// float height_multiplier
// sampler2D heights;
// mat4 shadow_view_projection


#if VertexShader

layout(location = 0) out highp vec3 world_pos;
layout(location = 1) out highp vec3 shadow_pos;
layout(location = 2) out highp vec2 uv;

void main()
{
  int x = gl_VertexID % width;
  int y = gl_VertexID / width;

  uv = vec2((float)x * rwidth, (float)y * rheight);

  float height = texture(heights, uv).x * height_multiplier;
  world_pos    = vec3(float(x), height, float(y));
  gl_Position = shadow_view_projection * vec4(world_pos, 1.0);
}

#elif FragmentShader

// pass through
void main()
{}

#endif