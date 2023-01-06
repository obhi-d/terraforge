
vec2 compute_input(float x, float y)
{
  return vec2(x, y);
}

void blur(in vec2 uv)
{
#ifdef HasBuffer_source
  float height = 0.0;
  if (blur_window == 0)
    height = texture(source, uv).x;
  else
  {
    for(int x = -blur_window; x < blur_window; ++x)
      for(int y = -blur_window; y < blur_window; ++y)
        height += texture(source, uv + vec2(float(x), float(y))*rsize).x;
    height *= blur_factor;
  }
#else
  float height = source;
#endif
  heights = height;
}

