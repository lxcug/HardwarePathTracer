#version 450

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

layout (location = 0) out vec3 fragColor;

layout (binding = 0) uniform dataPerFrame {
    mat4 modelTrans;
    mat4 viewTrans;
    mat4 projTrans;
};

void main() {
    gl_Position = vec4(aPos, 1.f);
    fragColor = aColor;
}
