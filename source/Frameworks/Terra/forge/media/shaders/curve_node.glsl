
layout(std430, location = source_b) readonly restrict buffer source
{
  uint  points;
  float c0;
  float values[];
}u_curve;

vec2 compute_input(float x, float y)
{
  return vec2(x, y);
}

uint closest_x(float x)
{   
  for(uint i = 1; i < u_curve.points; ++i)
  {
    if(x < u_curve.values[i])
      return i-1;
  }
  return u_curve.points-1;
}

float sample_x(float x)
{
  uint n   = u_curve.points;
  uint idx = closest_x( x );
  const uint sx = 0;
  const uint sy = n;
  const uint sb = n*2;
  const uint sc = n*3;
  const uint sd = n*4;

  float h = x - u_curve.values[idx];
  float interpol = 0.0;
  if( x < u_curve.values[0] )
  {
      // extrapolation to the left
    interpol = ( u_curve.c0 * h + u_curve.values[sb] ) * h + u_curve.values[sy];
  }
  else if( x > u_curve.values[n - 1] )
  {
      // extrapolation to the right
    interpol = ( u_curve.values[sc + n - 1] * h + u_curve.values[sb + n - 1] ) * h + u_curve.values[sy + n - 1];
  }
  else
  {
      // interpolation
    interpol = ( ( u_curve.values[sd + idx] * h + u_curve.values[sc + idx] ) * h + u_curve.values[sb + idx] ) * h + u_curve.values[sy + idx];
  }
  return interpol;
}

void node(in vec2 input, out float output)
{
  output = sample_x(input.x) * scale.x + sample_y(input.y) * scale.y;
}

