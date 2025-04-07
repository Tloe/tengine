#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 2, binding = 0) uniform sampler2D textures[];

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 2) flat in int fragTexIndex;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(textures[nonuniformEXT(fragTexIndex)], fragUV);
    outColor = texColor * fragColor;
}
