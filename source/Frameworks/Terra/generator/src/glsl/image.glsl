constexpr std::string_view gs_imageStore = R"_(

void store_{0}(vec4 value, NodeParams np)
{{
  ivec2 xy = ivec2(gl_GlobalInvocationID.xy);
  if (xy.x < int(np.env.size.x) && xy.y < int(np.env.size.y))
    imageStore({0}, xy, value);
}} 

)_";