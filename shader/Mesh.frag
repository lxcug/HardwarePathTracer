#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 texCoord;

layout (binding = 1) uniform sampler2D TexSampler;

layout (location = 0) out vec4 outColor;

void main() {
    vec3 TexColor = texture(TexSampler, texCoord).rgb;
    outColor = vec4(TexColor, 1.f);
}
