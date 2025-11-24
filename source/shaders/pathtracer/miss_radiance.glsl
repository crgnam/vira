// =======================
// Miss (Background) Shader
// =======================

#version 460 core
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

// =======================
// Ray payload
// =======================

struct DataPayload {

    vec4 receivedPower;         
    vec4 albedo;                
    vec4 directRadiance;        
    vec4 indirectRadiance;      
    vec4 normal;                

    vec2 uvInterpolated;

    float depth;                // TFloat

    uint samples;               // Number of samples
    uint instanceID;            // Instance ID
    uint meshID;                // Mesh ID
    uint pixelIndex;
    uint materialIndex;

    vec4 albedoSampled;
    vec4 emissionSampled;
    vec4 normalSampled;
    float roughnessSampled;
    float metalnessSampled;
    float transmissionSampled;
    float alpha;
};
layout(location = 0) rayPayloadInEXT DataPayload payload;

// =======================
// Descriptors
// =======================

// Descriptor Set 6: Debug Buffers
    layout(set = 6, binding = 1) buffer DebugCounter {
        uint debugCounterMiss;
    };

// =======================
// Main
// =======================

void main() {
    atomicAdd(debugCounterMiss, 1);
    payload.directRadiance = vec4(0.0, 0.0, 0.0, 0.0);
}