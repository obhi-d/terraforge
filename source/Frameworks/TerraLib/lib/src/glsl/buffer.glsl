constexpr std::string_view gs_bufferStore = R"_(

void store_{0}(output_t value, NodeParams np)
{{
  uint id = gl_GlobalInvocationID.x;
  {0}.data[id] = value;
}}

)_";

constexpr std::string_view gs_bufferLoad = R"_(

#if Has_TextureOutput

{0}_t sample_{0}(NodeParams np)
{{ 
  if (has_{0})
  {{
    
    uint id = get_pixel_id(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, np);
    return {0}.data[id];
  }}
  else
  {{
    return {0}_t(np.uniforms.{0});
  }}
}}

#else

{0}_t4 sample_{0}(NodeParams np)
{{ 
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
}}

#endif

{0}_t sample_{0}(uint x, uint y, NodeParams np)
{{ 
  if (has_{0})
  {{
    uint id = get_pixel_id(x, y, np);
    return {0}.data[id];
  }}
  else
  {{
    return np.uniforms.{0};
  }}
}}

{0}_t sample_{0}(uint id, NodeParams np)
{{ 
  if (has_{0})
  {{
    return {0}.data[id];
  }}
  else
  {{
    return np.uniforms.{0};
  }}
}}

{0}_t4 sample_{0}(uvec4 x, uvec4 y, NodeParams np)
{{ 
  if (has_{0})
  {{
    uint4 id = get_pixel_id(x, y, np);
    return {0}_t4(
      {0}.data[id.x],
      {0}.data[id.y],
      {0}.data[id.z],
      {0}.data[id.w]);
  }}
  else
  {{
    return {0}_t4(np.uniforms.{0}, np.uniforms.{0}, np.uniforms.{0}, np.uniforms.{0});
  }}
}}

)_";
