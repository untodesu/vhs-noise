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
    vec2 pv = vec2(0.0, 0.0);
    pv.x = clamp(params.x, 0.0, 1.0);
    pv.y = clamp(params.y, 0.0, 1.0);

    float noise = rand(100.0 * uv.x, 100.0 * uv.y);
    float nfx = rand(noise, uv.x) + 0.5;
    float nfy = rand(noise, uv.y) + 0.5;
    float nfz = rand(nfx, nfy) + 0.5;

    float thres = min(pv.x, 1.0 - 0.5 * pv.y * sin(3.14159265359 * pow(1.0125 - uv.y, 8.0)));
    float value = step(thres, noise);

    target.x = value * nfx;
    target.y = value * nfy;
    target.z = value * nfz;
    target.w = 1.0;
}
