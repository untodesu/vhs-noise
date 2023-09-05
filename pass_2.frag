#version 330 core

layout(location = 0) out vec4 target;

in vec2 uv;

uniform vec2 size;
uniform vec2 time;
uniform vec2 params;
uniform sampler2D pass;

const float steps = float(16);

void main(void)
{
    float step = 1.0 / size.x;

    target = vec4(0.0, 0.0, 0.0, 1.0);
    for(float i = 1.0; i <= steps; ++i)
        target.xyz += texture(pass, uv - vec2(i * step, 0.0)).xyz / i * 16.0;
    target.xyz += texture(pass, uv).xyz;
    target.xyz /= steps;
}
