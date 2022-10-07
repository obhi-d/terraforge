constexpr std::string_view gs_2dDecl = R"_(
  vec4 tint;
  mat4 projection;
)_";

constexpr std::string_view gs_2dVS = R"_(

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;

out vec4 v_color;
out vec2 v_uv;

void main()
{
  v_uv = uv;
  v_color = color * params.tint;
  gl_Position = params.projection * vec4(position, 0, 1);
}

)_";

constexpr std::string_view gs_2dFS = R"_(

in vec4 v_color;
in vec2 v_uv;
out vec4 fragColor;

void main()
{
  fragColor = v_color * params.tint * texture(diffuse, v_uv);
}

)_";
