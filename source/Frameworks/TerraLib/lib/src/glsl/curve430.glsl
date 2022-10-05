constexpr std::string_view gs_curve430 = R"_(

uint closest_{0}(float x, NodeParams np)
{{
  for(uint i = 1; i < {0}.npoints; ++i)
  {{
    if(x < {0}.data[i])
      return i-1;
  }}
  return {0}.npoints-1;
}}

float sample_{0}(float x, NodeParams np)
{{
  uint n   = {0}.npoints;
  uint idx = closest_{0}( x );
  const uint sx = 0;
  const uint sy = n;
  const uint sb = n*2;
  const uint sc = n*3;
  const uint sd = n*4;

  float h = x - {0}.data[idx];
  float interpol = 0.0;
  if( x < {0}.data[0] )
  {{
      // extrapolation to the left
      interpol = ( {0}.c0 * h + {0}.data[sb] ) * h + {0}.data[sy];
  }}
  else if( x > {0}.data[n - 1] )
  {{
      // extrapolation to the right
      interpol = ( {0}.data[sc + n - 1] * h + {0}.data[sb + n - 1] ) * h + {0}.data[sy + n - 1];
  }}
  else
  {{
      // interpolation
      interpol = ( ( {0}.data[sd + idx] * h + {0}.data[sc + idx] ) * h + {0}.data[sb + idx] ) * h + {0}.data[sy + idx];
  }}
  return interpol;
}}

float sample_{0}(float x, float y, NodeParams np)
{{
  return sample_{0}(x, np) + sample_{0}(y, np);
}}

vec4 sample_{0}(vec4 x, NodeParams np)
{{
  return vec4(sample_{0}(x.x, np), sample_{0}(x.y, np), sample_{0}(x.z, np), sample_{0}(x.w, np)); 
}}

vec4 sample_{0}(vec4 x, vec4 y, NodeParams np)
{{
  return sample_{0}(x, np) + sample_{0}(y, np); 
}}

)_";