

// uint width;
// uint height;
// float rwidth;
// float rheight;
// float height_multiplier
// sampler2D heights;
// mat4 shadow_view_projection


#ifdef VertexShader

layout(location = 1) out highp vec3 shadow_pos;

void main()
{
  uint x   = gl_VertexID % width;
  uint y   = gl_VertexID / width;
  float z   = texelFetch(heights, ivec2(x, y), 0).x;

  gl_Position = shadow_view_projection * vec4(float(x) - float(width - 1) * 0.5, z, float(y) - float(height - 1) * 0.5, 1.0);
}

#elif defined(FragmentShader)

// pass through
void main()
{}

#endif