constexpr std::string_view gs_MeshUBO = R"_(
  mat4      view_projection;
  int       width;
  int       height;
  float     style;
  float     frequency;
  vec3      sun_dir;
  float     height_multiplier;
  vec4      sun_color;
  vec4      tint;
  int       vertex_count;
  float     max_height;
  float     min_height;
  float     crust;

)_";

constexpr std::string_view gs_MeshVS = R"_(

layout(location = 0) in float heights;

layout(location = 0) out highp vec3 world_pos;

void main()
{
  ivec2 xy;
  float height;
  if (gl_VertexID >= constants.vertex_count)
  {
   xy.x = (gl_VertexID - constants.vertex_count) % constants.width;
   xy.y = (gl_VertexID - constants.vertex_count) / constants.width;
   height = (constants.min_height * constants.height_multiplier) - constants.crust;
  }
  else
  {
   xy.x = gl_VertexID % constants.width;
   xy.y = gl_VertexID / constants.width;
   height = heights * constants.height_multiplier;
  }
  world_pos = vec3(float(xy.x) - float(constants.width - 1) * 0.5, height, float(xy.y) - float(constants.height - 1) * 0.5);
  gl_Position = constants.view_projection * vec4(world_pos, 1.0);
}
)_";

constexpr std::string_view gs_MeshFS = R"_(

layout(location = 0) in highp vec3 world_pos;
layout(location = 0) out highp vec4 fragment_color;

void main()
{   
    vec3 x = dFdx(world_pos);
    vec3 y = dFdy(world_pos);
      
    vec3 normal = normalize(cross(x,y));
    
    float sampleHeight = ((world_pos.y / constants.height_multiplier)  - constants.min_height) / (constants.max_height - constants.min_height);
    float sampleNormal = clamp(normal.y, 0.0f, 1.0f); 
    vec4 color = texture(height_colors, vec2(sampleHeight, sampleNormal));

    vec4 sun = vec4(constants.sun_color.xyz, 1.0);
    float intensity = constants.sun_color.w;

    float factor = floor(clamp(dot(normal, constants.sun_dir), 0.0, 1.0) * float(constants.style)) / float(constants.style);
    fragment_color = mix(color, sun, 0.1) * factor * intensity;
}

)_";
