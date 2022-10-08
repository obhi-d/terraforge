
constexpr std::string_view gs_textureLoad = R"_(

                                                                                                            
float sample_{0}(float u, float v, uint x, uint y, NodeParams np)                                             
{{                                                                                                           
  if (has_{0})                                                                                              
  {{                                                                                                         
    if (is_tile_constrained_{0})                                                                            
    {{                                                                                                       
      if(!is_within_tile(x, y, np.uniforms.tile_vert_min_{0}, np.uniforms.tile_vert_max_{0}))               
        return np.uniforms.{0};                                                                             
    }}                                                                                                       
    return texture({0}, vec2(u.x * np.uniforms.uv_scale_{0}, v.x * np.uniforms.uv_scale_{0})).r;            
  }}                                                                                                         
  else                                                                                                      
  {{                                                                                                         
    return np.uniforms.{0};                                                                                 
  }}                                                                                                         
}}                                                                                                           
                                                                                                            
vec4 sample_{0}(vec4 u, vec4 v, uvec4 x, uvec4 y, NodeParams np)                                              
{{                                                                                                           
  return vec4(sample_{0}(u.x, v.x, x.x, y.x, np),                                                           
              sample_{0}(u.y, v.y, x.y, y.y, np),                                                           
              sample_{0}(u.z, v.z, x.z, y.z, np),                                                           
              sample_{0}(u.w, v.w, x.w, y.w, np));                                                          
}}                                                                                                           

)_";