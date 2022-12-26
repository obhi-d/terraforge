
vec2 compute_input(float x, float y)
{
  return vec2(x, y);
}

void node(in vec2 input, out float output)
{
  #ifdef HasImage_source
    output = texture(source, input * scale + offset).x;
  #else
    output = source;
  #endif
}
