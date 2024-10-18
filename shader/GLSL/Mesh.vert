#version 450

#include "ViewUniformBuffer.glsl"

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec2 texCoord;

void main() {
    gl_Position = ProjTrans * ViewTrans * ModelTrans * vec4(aPos, 1.f);
    texCoord = aTexCoord;
}
