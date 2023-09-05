#version 330 core

layout(location = 0) out vec4 target;

in vec2 uv;

uniform vec2 size;
uniform vec2 time;
uniform vec2 params;
uniform sampler2D pass;

const float steps = float(8);

float rand(float x, float y)
{
    /* I honestly cannot remember where I got this code from
     * but what matters is that it works and I'm good with it */
    return fract(sin(dot(vec3(x, y, time.y), vec3(12.9898, 78.233, 37.719))) * 143758.5453);
}

void main(void)
{
    float step = 1.0 / size.x;
    float factor = rand(uv.x, uv.y);
    float fsteps = steps + ceil(4.0 * factor);

    target = texture(pass, uv);

    for(float i = 0.1; i <= fsteps; ++i) {
        target.xyz += texture(pass, uv + vec2(i * step, 0.0)).xyz / i * 16.0 * factor;
        target.xyz += texture(pass, uv - vec2(i * step, 0.0)).xyz / i * 32.0 * factor;
    }

    target.xyz /= fsteps;
}
