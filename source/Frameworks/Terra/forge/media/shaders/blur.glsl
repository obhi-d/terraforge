
vec2 compute_input(float x, float y)
{
  return vec2(x, y);
}

void blur(in vec2 uv)
{
#ifdef HasSource_source
  float height = 0.0;
  for(int x = -blur_window; x <= blur_window; ++x)
    for(int y = -blur_window; y <= blur_window; ++y)
      height += blur_factor * texture(source, uv + vec2(float(x), float(y))*rsize).x;
  
#else
  float height = source;
#endif
  heights = height;
}

