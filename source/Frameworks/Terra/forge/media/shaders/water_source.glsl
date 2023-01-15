layout(local_size_x = 1, local_size_y = 1) in;


vec2 hash( vec2 p ) // replace this by something better
{
	p = vec2( dot(p, vec2(127.1, 11.7)), dot(p, vec2(269.5, 183.3)) );
	return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float noise( in vec2 p )
{
  const float K1 = 0.366025404; // (sqrt(3)-1)/2;
  const float K2 = 0.211324865; // (3-sqrt(3))/6;

	vec2  i = floor( p + (p.x + p.y) * K1 );
  vec2  a = p - i + (i.x + i.y) * K2;
  float m = step(a.y, a.x); 
  vec2  o = vec2(m, 1.0-m);
  vec2  b = a - o + K2;
	vec2  c = a - 1.0 + 2.0*K2;
  vec3  h = max( 0.5 - vec3(dot(a, a), dot(b, b), dot(c, c) ), 0.0 );
	vec3  n = h * h * h * h * vec3( dot(a, hash(i + 0.0)), dot(b, hash(i + o)), dot(c, hash(i + 1.0)));
  return dot( n, vec3(70.0) );
}

float irregular_blob(in vec2 st, in vec2 center, in float radius) 
{
  const uint octaves = 4;
  const float lacunarity = 2.0;
  const float gain = 0.5;

  vec2 dist = center - st;
  float dist_squared = dot(dist, dist);
  float fnoise = 0.0;
  float ampl = 1.0;
  float frequency = 1.0f;
  for (uint i = 0; i < octaves; i++) 
  {
    fnoise += ampl * (abs(noise(st * frequency + center + fseed)) - 0.5);
    frequency *= lacunarity;
    ampl *= gain;
  }
  return smoothstep(radius, radius * .8, dist_squared - fnoise * radius);
}

void main()
{   
  float water = imageLoad(water_d, ivec2(gl_GlobalInvocationID.xy)).x;
  water += abs(irregular_blob(vec2(gl_GlobalInvocationID.xy)/vec2(gl_NumWorkGroups.xy), water_spawn.xy, water_spawn.z)) * water_spawn.w;
  imageStore(water_d, ivec2(gl_GlobalInvocationID.xy), vec4(water)));
}
