layout(local_size_x = 1, local_size_y = 1) in;

float get_water_level()
{
  #ifdef HasSource_water_level
    return imageLoad(water_level, ivec2(gl_GlobalInvocationID.xy)).x;
  #else
    return water_level;
  #endif
}

float get_source()
{
  #ifdef HasSource_source
    return imageLoad(source, ivec2(gl_GlobalInvocationID.xy)).x;
  #else
    return source;
  #endif
}

void main()
{   
  imageStore(water_d, ivec2(gl_GlobalInvocationID.xy), vec4(max(get_water_level() - get_source(), 0.0)));
}
