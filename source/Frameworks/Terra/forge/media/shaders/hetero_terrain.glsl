
#define noisefn openSimplex2SDerivatives_Conventional_F


vec2 compute_input(float x, float y)
{
  return vec2((x * size.x) + start.x, (y * size.y) + start.y);
}

float random (float st) {
    return fract(sin(st * 12.9898) * 43758.5453123);
}

void noise(in vec2 p)
{
  float amp = amplitude;
  float freq = frequency;
  float seed = (fseed * random(fseed));
  float y = (noisefn(vec3(p * freq + vec2(fseed), fseed)) + offset);
  
  for(uint i = 1; i < octaves; ++i)
  {
    freq *= lacunarity;
    seed *= (seed * random(seed));
    float increment = (noisefn(vec3(p * freq + vec2(seed), fseed)) + offset)  * exponents[i];
    increment *= y;
    y += increment;
  }

  heights = amp * y;
}
