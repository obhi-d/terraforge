


vec2 compute_input(float x, float y)
{
  return vec2(x, y);
}

float sample_source_a(in vec2 uv)
{
  #ifdef HasSource_source_a
    return texelFetch(source_a, ivec2(gl_FragCoord.xy), 0).x;
  #else
    return source_a;
  #endif
}

float sample_source_b(in vec2 uv)
{
  #ifdef HasSource_source_b
    return texelFetch(source_b, ivec2(gl_FragCoord.xy), 0).x;
  #else
    return source_b;
  #endif
}

float sample_source_factor(in vec2 uv)
{
  #ifdef HasSource_source_factor
    return texelFetch(source_factor, ivec2(gl_FragCoord.xy), 0).x;
  #else
    return source_factor;
  #endif
}

void mixer(in vec2 uv, in float sa, in float sb, in float m)
{
  heights = mix(sa, sb, m);
}

