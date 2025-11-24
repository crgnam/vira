#version 460 core
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

// Shadow ray payload at location 1
layout(location = 1) rayPayloadEXT bool isShadowed;

void main() {
    isShadowed = false; // No occluder → point is lit
}
