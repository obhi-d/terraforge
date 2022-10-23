
constexpr std::string_view gs_textureLoad = R"_(

                                                                                                            
float sample_{0}(float u, float v, NodeParams np)                                             
{{                                                                                                           
  if (has_{0})                                                                                              
  {{                                                                                                         
    return texture({0}, vec2(u * np.uniforms.uv_scale_{0}.x + np.uniforms.uv_off_{0}.x, v * np.uniforms.uv_scale_{0}.y + np.uniforms.uv_off_{0}.y)).r;
  }}                                                                                                       
  else                                                                                                      
  {{                                                                                                         
    return np.uniforms.{0}.x;                                                                                 
  }}                                                                                                         
}}                                                                                                           
                                                                                                            
vec4 sample_{0}(vec4 u, vec4 v, NodeParams np)                                              
{{
  vec4 su = u * vec4(np.uniforms.uv_scale_{0}.x) + vec4(np.uniforms.uv_off_{0}.x);
  vec4 sv = v * vec4(np.uniforms.uv_scale_{0}.y) + vec4(np.uniforms.uv_off_{0}.y);
  return vec4(sample_{0}(u.x, v.x, np),                                                           
              sample_{0}(u.y, v.y, np),                                                           
              sample_{0}(u.z, v.z, np),                                                           
              sample_{0}(u.w, v.w, np));                                                          
}}                                                                                                           

)_";



constexpr std::string_view gs_textureArrayLoad = R"_(

                                                                                                            
float sample_{0}(float u, float v, float w, NodeParams np)                                             
{{                                                                                                           
  if (has_{0})                                                                                              
  {{                                                                                                         
    return texture({0}, vec3(u * np.uniforms.uv_scale_{0}.x + np.uniforms.uv_off_{0}.x, v * np.uniforms.uv_scale_{0}.y + np.uniforms.uv_off_{0}.y, w)).r;
  }}                                                                                                       
  else                                                                                                      
  {{                                                                                                         
    return np.uniforms.{0}.x;                                                                                 
  }}                                                                                                         
}}                                                                                                           
                                                                                                            
vec4 sample_{0}(vec4 u, vec4 v, vec4 w, NodeParams np)                                              
{{
  vec4 su = u * vec4(np.uniforms.uv_scale_{0}.x) + vec4(np.uniforms.uv_off_{0}.x);
  vec4 sv = v * vec4(np.uniforms.uv_scale_{0}.y) + vec4(np.uniforms.uv_off_{0}.y);
  return vec4(sample_{0}(u.x, v.x, w.x, np),                                                           
              sample_{0}(u.y, v.y, w.y, np),                                                           
              sample_{0}(u.z, v.z, w.z, np),                                                           
              sample_{0}(u.w, v.w, w.w, np));                                                          
}}                                                                                                           

)_";