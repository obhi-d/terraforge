
#define noisefn openSimplex2

vec2 compute_input(float x, float y)
{
  return vec2((x * size.x) + start.x, (y * size.y) + start.y);
}

void noise(in vec2 p)
{
  float amp = amplitude;
  float freq = frequency;
  float signal = abs(noisefn(vec3(p * freq, fseed)));
  signal = offset - signal;
  signal *= signal;
  float y = signal;
  float weight = 1.0;

  for(uint i = 1; i < octaves; ++i)
  {
    weight = clamp(signal * threshold, 0.0, 1.0);
    signal = abs(noisefn(vec3(p * freq, fseed)));
    signal = offset - signal;
    signal *= signal;
    signal *= weight;
    y += signal * pow( lacunarity, float(-i) * exponent ); 
    freq *= lacunarity;
  }

  heights = y;
}
