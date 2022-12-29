
// assume
// #define TextureSizeX
// #define Pass_Texture
// uniform sampler2D source;
// uniform uint pixel_count
// uniform uint block_size;
// uniform uint skip_block_size;

layout(local_size_x = 1) in;

#ifdef Pass_Texture

  layout(std430, binding = data_dst_b) restrict writeonly buffer data_dst {
    vec2 minmax[];
  }u_dst;

  vec2 sample_pixel(uint i)
  {
    int u = int(i % texture_size_x);
    int v = int(i / texture_size_x);
    return vec2(texelFetch(data_src, ivec2(u, v), 0).x);
  }

  void write_result(vec2 val, uint i)
  {
    u_dst.minmax[i] = val;
  }

#else

  layout(std430, binding = data_buffer_b) restrict buffer data_buffer {
    vec2 minmax[];
  }u_data;

  vec2 sample_pixel(uint i)
  {
    return u_data.minmax[i * skip_block_size];
  }
    
  void write_result(vec2 val, uint i)
  {
    u_data.minmax[i] = val;
  }

#endif

void main()
{   
    uint start_pix = uint(gl_GlobalInvocationID.x) * block_size;
    uint end_pix   = min(pixel_count, block_size + start_pix);
    vec2 minmax    = sample_pixel(start_pix);
    for(uint i = start_pix + 1; i < end_pix; ++i)
    {
      vec2 s = sample_pixel(i);
      minmax.x = min(minmax.x, s.x);
      minmax.y = max(minmax.y, s.y);
    }

    write_result(minmax, uint(gl_GlobalInvocationID.x));
}

