#version 450

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 proj;
} globalUBO;

layout(set = 1, binding = 0) uniform ModelUBO {
    mat4 model;
    int textureIndex;
} modelUBO;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragUV;
layout(location = 2) flat out int fragTexIndex;

void main() {
    gl_Position = globalUBO.proj * modelUBO.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
    fragUV = inUV;
    fragTexIndex = modelUBO.textureIndex;
}
