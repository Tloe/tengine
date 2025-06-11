#version 450

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 proj;
} globalUBO;

layout(push_constant) uniform ModelPC {
    mat4 model;
    int textureIndex;
} model_constants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) flat out int fragTexIndex;

void main() {
    gl_Position = globalUBO.proj * globalUBO.view * model_constants.model * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    fragTexIndex = model_constants.textureIndex;
}
