constexpr std::string_view gs_bufferStore = R"glsl(

void store_{0}(output_t value, NodeParams np)
{{
  uint id = gl_GlobalInvocationID.x;
  {0}.data[id] = value;
}}

)glsl";

constexpr std::string_view gs_bufferLoad = R"glsl(

{0}_t4 sample_{0}(NodeParams np)
{{ 
#ifdef {0}_FromSnippet
  return {0}_FromSnippet(np);
#else
  if (has_{0})
  {{
    
    uint id = gl_GlobalInvocationID.x;
    id *= 4;
    return {0}_t4({0}.data[id + 0], 
                {0}.data[id + 1], 
                {0}.data[id + 2], 
                {0}.data[id + 3]);
  }}
  else
  {{
    return {0}_t4(np.uniforms.{0}, np.uniforms.{0}, np.uniforms.{0}, np.uniforms.{0});
  }}
#endif
}}

)glsl";
