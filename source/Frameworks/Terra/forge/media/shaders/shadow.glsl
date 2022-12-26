

// uint width;
// uint height;
// float rwidth;
// float rheight;
// float height_multiplier
// sampler2D heights;
// mat4 shadow_view_projection


#ifdef VertexShader

layout(location = 0) out highp vec3 world_pos;
layout(location = 1) out highp vec3 shadow_pos;
layout(location = 2) out highp vec2 uv;

void main()
{
  uint x = gl_VertexID % width;
  uint y = gl_VertexID / width;

  uv = vec2(float(x) * rwidth, float(y) * rheight);

  float z   = (texture(heights, uv).x * height_multiplier);
  world_pos = vec3(float(x) - float(width - 1) * 0.5, z, float(y) - float(height - 1) * 0.5);
  gl_Position = shadow_view_projection * vec4(world_pos, 1.0);
}

#elif defined(FragmentShader)

// pass through
void main()
{}

#endif