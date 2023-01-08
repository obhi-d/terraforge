
vec2 compute_input(float x, float y)
{
  return vec2(x, y);
}

void smoothen(in vec2 uv)
{
#ifdef HasSource_source_a
  float src_a = input_scale * texelFetch(source_a, ivec2(gl_FragCoord.xy), 0).x;
#else
  float src_a = source_a;
#endif
#ifdef HasSource_source_b
  float src_b = input_scale * texelFetch(source_b, ivec2(gl_FragCoord.xy), 0).x;
#else
  float src_b = source_b;
#endif
#ifdef HasSource_smooth_factor
  float smoothness = texelFetch(smooth_factor, ivec2(gl_FragCoord.xy), 0).x;
#else
  float smoothness = smooth_factor;
#endif

  float c = max(0.000001, smoothness);
  float h = max(c - abs(src_a - src_b), 0.0) / c;
  heights = output_scale * (min(src_a, src_b) - h * h * h * c * (1.0 / 6.0));
}

