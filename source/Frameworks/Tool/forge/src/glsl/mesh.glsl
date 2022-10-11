constexpr std::string_view gs_MeshUBO = R"_(
layout(binding = 0) uniform Constants
{
  int width;
  int height;
  int style;
  float frequency;
  vec3 sun_dir;
  float height_multiplier;
  vec4 sun_color;
  vec4 tint;
  mat4 view_projection;
}constants;
)_";

constexpr std::string_view gs_MeshRes = R"_(
layout(binding = 0) uniform sampler2D height_colors;
)_";

constexpr std::string_view gs_MeshVS = R"_(

layout(location = 0) in float heights;

layout(location = 0) out highp vec3 world_pos;

void main()
{
  int x = gl_VertexID % constants.width;
  int y = gl_VertexID / constants.width;
  world_pos = vec4(float(x) * constants.frequency, heights * constants.height_multiplier, float(y) * constants.frequency);
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
    
    vec4 light_x = texture(height_colors, vec2(clamp(normal.x * 0.5 + 0.5, 0.0, 1.0), 0.01 * abs(normal.x)));
    vec4 light_y = texture(height_colors, vec2(clamp(world_pos.y, 0.0, 1.0), 0.5 * normal.y));
    vec4 light_z = texture(height_colors, vec2(clamp(normal.z * 0.5 + 0.5, 0.0, 1.0), 0.99 * normal.z));

    if(!gl_FrontFacing) 
    { 
        light_y = (1.0 - light_y) * 0.08;
    }

    vec4 color = vec4(constants.sun_color.xyz, 1.0);
    float intensity = constants.sun_color.w;

    float factor = floor(clamp(dot(normal, constants.sun_dir), 0.0, 1.0) * float(constants.style)) / float(constants.style);
    fragment_color = mix(color, light_x * .4 + light_y * .6 + light_z * .4, 0.8) * factor * intensity;
}

)_";
