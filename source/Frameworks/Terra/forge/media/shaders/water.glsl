


// uint width;
// uint height;
// float rwidth;
// float rheight;
// float height_multiplier
// vec4 sun_data;
// vec4 layer_weights;
// mat4 shadow_view_projection
// mat4 view_projection
// sampler1DArray layer_colors;
// sampler2D heights;
// sampler2DShadow shadow_map;
// sampler2D layers;

#if defined(VertexShader)

layout(location = 0) out highp vec2 uv;

const float x[4] = {-1.0, 1.0, -1.0, 1.0};
const float y[4] = {-1.0,-1.0,  1.0, 1.0};
void main()
{
  // float z   = hscale * texelFetch(heights, ivec2(x, y), 0).x;
  uv = vec2((x[gl_VertexID] + 1.0) * 0.5, (y[gl_VertexID] + 1.0) * 0.5);
  gl_Position = view_projection * vec4(x[gl_VertexID] * width, hscale * water_level, y[gl_VertexID] * height, 1.0);
}

#elif defined(FragmentShader)

layout(location = 0) in highp vec2 uv;

// Author @patriciogv - 2015
// http://patriciogonzalezvivo.com
// Author @patriciogv - 2015
// http://patriciogonzalezvivo.com


vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }

float snoise(vec2 v) {
    const vec4 C = vec4(0.211324865405187,  // (3.0-sqrt(3.0))/6.0
                        0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
                        -0.577350269189626,  // -1.0 + 2.0 * C.x
                        0.024390243902439); // 1.0 / 41.0
    vec2 i  = floor(v + dot(v, C.yy) );
    vec2 x0 = v -   i + dot(i, C.xx);
    vec2 i1;
    i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec4 x12 = x0.xyxy + C.xxzz;
    x12.xy -= i1;
    i = mod289(i); // Avoid truncation effects in permutation
    vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
        + i.x + vec3(0.0, i1.x, 1.0 ));

    vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
    m = m*m ;
    m = m*m ;
    vec3 x = 2.0 * fract(p * C.www) - 1.0;
    vec3 h = abs(x) - 0.5;
    vec3 ox = floor(x + 0.5);
    vec3 a0 = x - ox;
    m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
    vec3 g;
    g.x  = a0.x  * x0.x  + h.x  * x0.y;
    g.yz = a0.yz * x12.xz + h.yz * x12.yw;
    return 130.0 * dot(m, g);
}

float swirl(vec2 st, float u_time) {
    vec4 k = vec4(u_time)*0.8;
    k.xy = st * 7.0;
    float val1 = length(0.5-fract(k.xyw*=mat3(vec3(-2.0,-1.0,0.0), vec3(3.0,-1.0,1.0), vec3(1.0,-1.0,-1.0))*0.5));
    float val2 = length(0.5-fract(k.xyw*=mat3(vec3(-2.0,-1.0,0.0), vec3(3.0,-1.0,1.0), vec3(1.0,-1.0,-1.0))*0.2));
    float val3 = length(0.5-fract(k.xyw*=mat3(vec3(-2.0,-1.0,0.0), vec3(3.0,-1.0,1.0), vec3(1.0,-1.0,-1.0))*0.5));
    return pow(min(min(val1,val2),val3), 7.0) * 2.0;
}

void main()
{      
    //background texture
   	//vec4 texture_color = texture(iChannel0, uv);
    float height = texture(heights, uv).x;
    float diff = water_level - height;
    float alpha = 0.5;
    if (diff < -0.3)
     alpha = exp(diff * 10);
    diff = abs(diff) * .5;
   	//background color rgb( 49/255, 169/255, 238/255, 255/255 ) -- 0.192156862745098, 0.6627450980392157, 0.9333333333333333
    float tt = ftime / 100.f;
    float water_contrib = 1 - (exp(-diff * 5.1) - snoise((uv + tt) * wave_period * 10.f) * 0.1);
    vec3 texture_color = texture(layer_colors, vec2(water_contrib, 0.0)).xyz;
    
    color_buffer = vec4(texture_color + swirl(uv * 20.f, ftime), alpha);  
}

#endif