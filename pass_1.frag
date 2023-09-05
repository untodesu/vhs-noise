#version 330 core

layout(location = 0) out vec4 target;

in vec2 uv;

uniform vec2 size;
uniform vec2 time;
uniform vec2 params;
uniform sampler2D pass; /* unused */

float rand(float x, float y)
{
    /* I honestly cannot remember where I got this code from
     * but what matters is that it works and I'm good with it */
    return fract(sin(dot(vec3(x, y, time.y), vec3(12.9898, 78.233, 37.719))) * 143758.5453);
}

void main(void)
{
    float noise = rand(uv.x, uv.y);
    float thres = min(params.x, 1.0 - 0.5 * params.y * sin(3.14159265359 * pow(1.025 - uv.y, 8.0)));
    float value = step(thres, noise);
    vec4 prev = texture(pass, uv);
    target.x = value;
    target.y = value;
    target.z = value;
    target.w = 1.0;
}
