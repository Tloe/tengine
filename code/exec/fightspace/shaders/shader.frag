#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_debug_printf : enable

const uint MATERIAL_BORDER = 0u;
const uint MATERIAL_AIR    = 1u;
const uint MATERIAL_SAND   = 2u;
const uint MATERIAL_WATER  = 3u;
const uint MATERIAL_WOOD   = 4u;
const uint MATERIAL_STONE  = 5u;
const uint MATERIAL_FIRE   = 6u;

const vec3 MATERIAL_COLORS[7] = vec3[7](
    vec3(0.0, 0.0, 0.0),         // BORDER
    vec3(51.0, 204.0, 255.0),    // AIR
    vec3(255.0, 204.0, 102.0),   // SAND
    vec3(0.0, 102.0, 204.0),     // WATER
    vec3(153.0, 102.0, 51.0),    // WOOD
    vec3(153.0, 153.0, 153.0),   // STONE
    vec3(255.0, 102.0, 0.0)      // FIRE
);

const ivec2 SIM_RES = ivec2(192, 108);

const uint UPSCALE = 10u; // TODO : make this a uniform

layout(set = 0, binding = 0) uniform sampler2D textures[];
layout(set = 1, binding = 0) readonly buffer MaterialTypeBuffer{
    uint materials[];
};

layout(location = 0) out vec4 outColor;

void main() {
    // Get screen-space pixel coordinate
    ivec2 screenPx = ivec2(gl_FragCoord.xy);

    // Map to simulation pixel (integer division = floor)
    ivec2 simPx = screenPx / int(UPSCALE);

    // Bounds check (in case screen resolution isn't an exact multiple)
    if (simPx.x >= SIM_RES.x || simPx.y >= SIM_RES.y) {
        discard;
    }

    // Convert 2D sim coordinate to 1D index into SSBO
    int index = simPx.y * SIM_RES.x + simPx.x;

    // Lookup material
    uint mat = materials[index];

    // Output color
    outColor = vec4(MATERIAL_COLORS[mat], 1.0);
}
