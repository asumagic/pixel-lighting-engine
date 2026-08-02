#version 330 core

uniform sampler2D screen_texture;
uniform sampler2D palette_texture;
uniform sampler2D lightmap_texture;
uniform sampler2D palette_shift_texture;

// https://www.shadertoy.com/view/ttcyRS

vec3 oklab_mix( vec3 colA, vec3 colB, float h )
{
    // https://bottosson.github.io/posts/oklab
    const mat3 kCONEtoLMS = mat3(                
         0.4121656120,  0.2118591070,  0.0883097947,
         0.5362752080,  0.6807189584,  0.2818474174,
         0.0514575653,  0.1074065790,  0.6302613616);
    const mat3 kLMStoCONE = mat3(
         4.0767245293, -1.2681437731, -0.0041119885,
        -3.3072168827,  2.6093323231, -0.7034763098,
         0.2307590544, -0.3411344290,  1.7068625689);
                    
    // rgb to cone (arg of pow can't be negative)
    vec3 lmsA = pow( kCONEtoLMS*colA, vec3(1.0/3.0) );
    vec3 lmsB = pow( kCONEtoLMS*colB, vec3(1.0/3.0) );
    // lerp
    vec3 lms = mix( lmsA, lmsB, h );
    // gain in the middle (no oaklab anymore, but looks better?)
 // lms *= 1.0+0.2*h*(1.0-h);
    // cone to rgb
    return kLMStoCONE*(lms*lms*lms);
}

uint screen_color_indexed()
{
	return uint(texelFetch(screen_texture, ivec2(gl_FragCoord.xy), 0).r * 255.0);
}

vec4 indexed_to_rgb(uint index)
{
	return texelFetch(palette_texture, ivec2(index, 0), 0);
}

float light_level()
{
	return texelFetch(lightmap_texture, ivec2(gl_FragCoord.xy), 0).r;
}

vec4 apply_lighting(uint index, float light)
{
	vec2 pxScale = vec2(1.0 / 8.0, 1.0 / 32.0);
	vec2 basePos = vec2(light / pxScale.x, float(index));
	
	uint shiftIndex1 = uint(texelFetch(palette_shift_texture, ivec2(clamp(basePos.x - 0.5, 0, 7), basePos.y), 0).r * 255.0);
	uint shiftIndex2 = uint(texelFetch(palette_shift_texture, ivec2(clamp(basePos.x + 0.5, 0, 7), basePos.y), 0).r * 255.0);
	
	vec4 color1 = indexed_to_rgb(shiftIndex1);
	vec4 color2 = indexed_to_rgb(shiftIndex2);

	float interpolation = fract((light * 8) - 0.5);

	return vec4(
		oklab_mix(color1.rgb, color2.rgb, interpolation),
		mix(color1.a, color2.a, interpolation)
	);
}

void main()
{
	uint index = screen_color_indexed();
	gl_FragColor = apply_lighting(index, light_level());
}
