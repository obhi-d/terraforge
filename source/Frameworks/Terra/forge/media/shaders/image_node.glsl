
vec2 compute_input(float x, float y)
{
  return vec2(x, y);
}

void node(in vec2 uv)
{
  #ifdef HasImage_source
    heights = texture(source, uv * sample_scale + sample_offset).x * amplitude;
  #else
    heights = amplitude;
  #endif
}
