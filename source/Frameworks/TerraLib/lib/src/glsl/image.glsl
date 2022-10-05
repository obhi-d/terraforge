constexpr std::string_view gs_imageStore = R"_(

void store_{0}(vec4 value, NodeParams np)
{{
  uvec2 xy = gl_GlobalInvocationID.xy;
  if (xy.x < np.env.size.x && xy.y < np.env.size.y)
    imageStore({0}, xy, value);
}} 

)_";