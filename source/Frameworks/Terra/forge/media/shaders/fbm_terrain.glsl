
#define noisefn openSimplex2

vec2 compute_input(float x, float y)
{
  return vec2((x * size.x) + start.x, (y * size.y) + start.y);
}

void noise(in vec2 p)
{
  float amp = amplitude;
  float freq = frequency;
  float y = 0;
  for(uint i = 0; i < octaves; ++i)
  {
    y += pow(amp, exponent) * noisefn(vec3(p * freq + vec2(fseed * fseed), fseed));
    freq *= lacunarity;
    amp *= gain;
  }

  heights = y;
}
