constexpr std::string_view gs_2dDecl = R"_(

struct Params
{
  vec4 tint;
  vec2 scale;
  vec2 offset;
};

)_";

constexpr std::string_view gs_2dVS = R"_(

layout(location = 0) in vec2 position;

#if HasColor
  layout(location = 1) in vec4 color;
  out vec4 v_color;
#endif

#if HasTexture
  layout(location = 2) in vec2 uv;
  out vec2 v_uv;
#endif

void main()
{
  #if HasTexture
    v_uv = uv;
  #endif
  #if HasColor
    v_color = color * params.tint;
  #endif
  gl_Position = position * params.scale + params.offset;
}

)_";

constexpr std::string_view gs_2dFS = R"_(

#if HasColor
  in vec4 v_color;
#endif
#if HasTexture
  in vec2 v_uv
#endif

void main()
{
  vec4 color;
#if HasColor
  color = v_color;
#else
  color = params.tint;
#endif  
#if HasTexture
  color *= texture(diffuse, v_uv);
#endif
  gl_FragColor = color;
}

)_";
