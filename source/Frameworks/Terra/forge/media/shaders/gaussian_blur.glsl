
layout(std430, binding = gauss_blur_factor_b) restrict readonly buffer gauss_blur_factor {
  uint ksize;
  vec3 dir_factor[];
}u_gbf;


vec2 compute_input(float x, float y)
{
  return vec2(x, y);
}

void blur(in vec2 uv)
{
#ifdef HasSource_source
  float height = 0.0;
  for(uint i = 0; i < u_gbf.ksize; ++i)
    height += u_gbf.dir_factor[i].z * texture(source, uv + u_gbf.dir_factor[i].xy*rsize).x;  
#else
  float height = source;
#endif
  heights = height;
}

