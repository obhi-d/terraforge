


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

// https://www.shadertoy.com/view/MstXWn
vec2 hash( vec2 p )
{
	p = vec2( dot(p,vec2(127.1,311.7)),
			  dot(p,vec2(269.5,183.3)) );

	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float level=1.;
float noise( in vec2 p )
{
  vec2 i = floor( p );
  vec2 f = fract( p );
	
	vec2 u = f*f*(3.0-2.0*f);
    float t = pow(2.,level)* .4*ftime;
    mat2 R = mat2(cos(t),-sin(t),sin(t),cos(t));
    if (mod(i.x+i.y,2.)==0.) R=-R;

    return 2.*mix( mix( dot( hash( i + vec2(0,0) ), (f - vec2(0,0))*R ), 
                     dot( hash( i + vec2(1,0) ),-(f - vec2(1,0))*R ), u.x),
                mix( dot( hash( i + vec2(0,1) ),-(f - vec2(0,1))*R ), 
                     dot( hash( i + vec2(1,1) ), (f - vec2(1,1))*R ), u.x), u.y);
}

float turb( in vec2 uv )
{ 	float f = 0.0;
	
   level=1.;
    mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
    f  = 0.5000*noise( uv ); uv = m*uv; level++;
	f += 0.2500*noise( uv ); uv = m*uv; level++;
	f += 0.1250*noise( uv ); uv = m*uv; level++;
	f += 0.0625*noise( uv ); uv = m*uv; level++;
	return f/.9375; 
}

void main()
{   
  // float height = 0.6 * texture(heights, uv).x;
  // height += 0.1 * texture(heights, uv + rscale).x;
  // height += 0.1 * texture(heights, uv - rscale).x;
  // height += 0.1 * texture(heights, uv + vec2(rscale.x, -rscale.y)).x;
  // height += 0.1 * texture(heights, uv + vec2(-rscale.x, rscale.y)).x;
  // float alpha = 0.5;
  // if (water_level < height)
  //   alpha = 0.01;
  float alpha = 0.5;
  float water_contrib = turb(wave_period * uv) * 0.5 + 0.5;
  color_buffer = vec4(texture(layer_colors, vec2(water_contrib, 0.0)).xyz, alpha);
}

#endif