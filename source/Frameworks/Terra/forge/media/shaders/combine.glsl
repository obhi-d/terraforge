layout(local_size_x = 1, local_size_y = 1) in;

float get_a()
{
  #ifdef HasSource_source_a
    return imageLoad(source_a, ivec2(gl_GlobalInvocationID.xy)).x;
  #else
    return source_a;
  #endif
}

float get_b()
{
  #ifdef HasSource_source_b
    return imageLoad(source_b, ivec2(gl_GlobalInvocationID.xy)).x;
  #else
    return source_b;
  #endif
}


float get_scale_a()
{
  #ifdef HasSource_scale_a
    return imageLoad(scale_a, ivec2(gl_GlobalInvocationID.xy)).x;
  #else
    return scale_a;
  #endif
}

float get_scale_b()
{
  #ifdef HasSource_scale_b
    return imageLoad(scale_b, ivec2(gl_GlobalInvocationID.xy)).x;
  #else
    return scale_b;
  #endif
}

float combine(float a, float b)
{
#if defined(Enum_Multiply)
  return a * b;
#elif defined(Enum_Divide)
  return a / b;
#elif defined(Enum_Power)
  return pow(abs(a), abs(b));
#elif defined(Enum_AbsAdd)
  return abs(a) + abs(b);
#else
  return a + b;
#endif
}

void main()
{   
  imageStore(heights, ivec2(gl_GlobalInvocationID.xy), vec4(combine(get_a() * get_scale_a(), get_b() * get_scale_b())));
}