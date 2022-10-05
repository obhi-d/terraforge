constexpr std::string_view gs_bufferStore = R"_(

void store_{0}(float4 value, NodeParams np)
{{
  uint id = gl_GlobalInvocationID.x;
  {0}.data[id] = value;
}}

)_";

constexpr std::string_view gs_bufferLoad430 = R"_(

vec4 sample_{0}(NodeParams np)
{{ 
  if (has_{0})
  {{
    uint id = gl_GlobalInvocationID.x;
    id *= 4;
    return vec4({0}.data[id + 0], 
                {0}.data[id + 1], 
                {0}.data[id + 2], 
                {0}.data[id + 3]);
  }}
  else
  {{
    return vec4(np.uniforms.{0});
  }}
}}

float sample_{0}(uint x, uint y, NodeParams np)
{{ 
  if (has_{0})
  {{
    uint id = pixel_id(x, y, np);
    return {0}.data[id];
  }}
  else
  {{
    return np.uniforms.{0};
  }}
}}

vec4 sample_{0}(uint4 x, uint4 y, NodeParams np)
{{ 
  if (has_{0})
  {{
    int4 id = pixel_id(x, y, np);
    return vec4(
      {0}.data[id.x],
      {0}.data[id.y],
      {0}.data[id.z],
      {0}.data[id.w]);
  }}
  else
  {{
    return vec4(np.uniforms.{0});
  }}
}}

)_";

constexpr std::string_view gs_bufferLoad140 = R"_(

vec4 sample_{0}(NodeParams np)
{{ 
  if (has_{0})
  {{
    uint id = gl_GlobalInvocationID.x;
    return {0}.data[id];
  }}
  else
  {{
    return vec4(np.uniforms.{0});
  }}
}}

float sample_{0}(uint x, uint y, NodeParams np)
{{ 
  if (has_{0})
  {{
    uint id = pixel_id(x, y, np);
    return {0}.data[id][id % 3];
  }}
  else
  {{
    return np.uniforms.{0};
  }}
}}

vec4 sample_{0}(uint4 x, uint4 y, NodeParams np)
{{ 
  if (has_{0})
  {{
    uint4 id = pixel_id(x, y, np);
    uint4 px = id % 4;
    return vec4(
      {0}.data[id.x][px.x],
      {0}.data[id.y][px.y],
      {0}.data[id.z][px.z],
      {0}.data[id.w][px.w]);
  }}
  else
  {{
    return vec4(np.uniforms.{0});
  }}
}}

)_";