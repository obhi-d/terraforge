
vec2 compute_input(float x, float y)
{
  return vec2(x, y);
}

void rescale(in vec2 uv)
{
#ifdef HasSource_source
  float height = texelFetch(source, ivec2(gl_FragCoord.xy), 0).x;
  heights = ((height - minmax.x) * minmax.y) * (scale_max - scale_min) + scale_min;
#else
  heights = source;
#endif
}

