// =======================
// Closest Hit Shader
// =======================

#version 460 core
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

// =======================
// Constants
// =======================
#define PI 3.14159265359

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
layout(location = 1) rayPayloadEXT bool isShadowed;

// =======================
// Hit Attribute
// =======================
hitAttributeEXT vec2 bary;

// =======================
// Push Constants
// =======================
layout(push_constant) uniform PushConstants {
    uint nSpectralBands;
    uint nLights;
} pc;

// =======================
// Descriptors
// =======================

// == Descriptor Set 0: Camera ==
// Binding 0: Camera Properties
layout(set = 0, binding = 0) uniform Camera {
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    mat4 projectionMatrix;
    mat4 normalMatrix;
    mat4 intrinsicMatrix;
    mat4 inverseIntrinsicMatrix;
    vec3 cameraPosition;
    float padding1;
    float focalLength;
    float aperture;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    float focusDistance;
    float exposure;
    float zDir;
    bool  depthOfField;
    float k1;
    float k2;
    float k3;
    float k4;
    float k5;
    float k6;  // Radial distortion
    float p1;
    float p2;                  // Tangential distortion
    float s1;
    float s2;
    float s3;
    float s4;          // Thin prism distortion
    ivec2 resolution;

} camera;

// Binding 1: Pixel Solid Angle Map Texture
layout(set = 0, binding = 1) uniform sampler2D pixelSolidAngleTexture;

// Binding 2: Optical Efficiency
layout(set = 0, binding = 2) buffer OpticalEfficiencyBuffer {
    float opticalEfficiency[];  // Assuming TFloat is float
};

// == Descriptor Set 1: Material Info and Textures == 

struct MaterialInfo {
    int bsdfType;
    int samplingMethod;  // Ensure vira::SamplingMethod maps correctly to int
};

// Binding 0: Uniform Buffer containing an array of MaterialInfo
layout(set = 1, binding = 0) buffer MaterialBuffer {
    MaterialInfo materials[];
};

// Bindings 1-6: Texture samplers for various material maps
layout(set = 1, binding = 1) uniform sampler2D normalTextures[];
layout(set = 1, binding = 2) uniform sampler2D roughnessTextures[];
layout(set = 1, binding = 3) uniform sampler2D metalnessTextures[];
layout(set = 1, binding = 4) uniform sampler2D transmissionTextures[];
layout(set = 1, binding = 5) uniform sampler2D albedoTextures[];
layout(set = 1, binding = 6) uniform sampler2D emissionTextures[];


// == Descriptor Set 2: Light Resources == 
// Binding 0: Light Parameters
layout(set = 2, binding = 0) uniform LightData {
    vec3 position;          // Light position
    float type;             // Light type identifier (0 = point, 1 = sphere)
    vec4 color;             // Light color/intensity
    float radius;           // Light radius
} lights[];

// == Descriptor Set 3: Geometry (TLAS) ==
// Binding 0: TLAS
layout(set = 3, binding = 0) uniform accelerationStructureEXT tlas;

// == Descriptor Set 4: Mesh Resources (Vertices, Indices, Material IDs, Material Offsets) ==
struct VertexVec4 {
    vec3 position;
    float padding0;
    vec4 albedo;
    vec3 normal;
    float padding1;
    vec2 uv;
    float padding2[2];  // 8-byte padding for 16-byte alignment
};

// Binding 0: Vertex Buffers 
layout(set = 4, binding = 0) buffer VertexBuffers {
    VertexVec4 vertices[];
} vertexBufferArray[];

// Binding 1: Index Array
layout(set = 4, binding = 1) buffer IndexArray {
    uint indices[];
} indexArray[];

// Binding 2: Per-face materials
layout(set = 4, binding = 2) buffer FaceMaterialBuffers {
    uint materialIDs[];
} faceMaterialIDArray[];

// Binding 3: Material Offsets
layout(set = 4, binding = 3) buffer MaterialOffsets {
    uint materialOffsets[];
};

// == Descriptor Set 6: Debug Buffers ==
// Binding 2: Hit Shader Invocation DebugCounter
layout(set = 6, binding = 2) buffer DebugCounter {
    uint debugCounterHit;
};

// Binding 3: Debug Vector
layout(set = 6, binding = 3) buffer DebugVector {
    vec4 debugVec4[];
};


// =======================
// Shader Functions
// =======================
vec4 interpolateVec4(vec4 v0, vec4 v1, vec4 v2, vec2 bary) {
    return (1.0 - bary.x - bary.y) * v0 +
        bary.x * v1 +
        bary.y * v2;
}

vec3 interpolateVec3(vec3 v0, vec3 v1, vec3 v2, vec2 bary) {
    return (1.0 - bary.x - bary.y) * v0 +
        bary.x * v1 +
        bary.y * v2;
}

vec2 interpolateVec2(vec2 uv0, vec2 uv1, vec2 uv2, vec2 bary) {
    return (1.0 - bary.x - bary.y) * uv0 +
        bary.x * uv1 +
        bary.y * uv2;
}

// Function to calculate the solid angle of a light
float calculateLightPDF(vec3 lightPos, vec3 fragPos, float radius) {
    vec3 pointToLight = lightPos - fragPos;
    float distance = length(pointToLight);
    float r2 = radius * radius;
    float omega = 2.0 * PI * (1.0 - sqrt((distance * distance) - r2) / distance);
    return 1.0 / omega;
}

// McEwan (Lambertian + Lommel-Seeliger) BSDF
vec4 evaluateBSDF(vec4 radiance, vec4 albedo, vec3 normal, vec3 lightDir, vec3 viewDir) {
    float cosI = max(dot(normal, lightDir), 0.0);

    if (cosI == 0.0) {
        return vec4(0.0);
    }

    // Lambertian component
    float lambert = (1.0 / PI) * cosI;

    // Lommel-Seeliger component
    float cosE = max(dot(normal, viewDir), 0.0);
    float lommelSeeliger = (1.0 / (4.0 * PI)) * (cosI / (cosI + cosE));

    // Mixing factor
    float alpha0 = 60.0; // Phase angle adjustment
    float alpha = degrees(acos(dot(lightDir, viewDir))); // Phase angle
    float beta = exp(-alpha / alpha0);

    // Compute final reflectance
    return radiance * albedo * mix(lambert, lommelSeeliger, beta);
}   

// =======================
// Main
// =======================

void main() {

    atomicAdd(debugCounterHit, 1);

    // =======================
    // Data Retrieval
    // =======================

    uint blasIndex = gl_InstanceCustomIndexEXT; 
    uint triIndex = gl_PrimitiveID;
    vec3 rayOriginWorld = gl_WorldRayOriginEXT;
    vec3 rayDirectionWorld = gl_WorldRayDirectionEXT;
    
    vec2 baryCoords = bary;

    uint pixelIndex = payload.pixelIndex;

    int imageWidth = int(camera.resolution.x);
    int imageHeight = int(camera.resolution.y);

    // =======================
    // Material Indexing
    // =======================

    uint materialOffset = materialOffsets[nonuniformEXT(blasIndex)];
    uint faceMatID = faceMaterialIDArray[nonuniformEXT(blasIndex)].materialIDs[nonuniformEXT(triIndex)];
    uint materialIndex = materialOffset + faceMatID;

    // =======================
    // Vertex Interpolation
    // =======================

    uvec3 vertexIndices = uvec3(
        indexArray[nonuniformEXT(blasIndex)].indices[3*nonuniformEXT(triIndex)], 
        indexArray[nonuniformEXT(blasIndex)].indices[3*nonuniformEXT(triIndex)+1], 
        indexArray[nonuniformEXT(blasIndex)].indices[3*nonuniformEXT(triIndex)+2]
    );

    // Get vertex info (albedo and normal) for triangle vertices
    vec3 position0 = vertexBufferArray[nonuniformEXT(blasIndex)].vertices[nonuniformEXT(vertexIndices[0])].position;
    vec3 position1 = vertexBufferArray[nonuniformEXT(blasIndex)].vertices[nonuniformEXT(vertexIndices[1])].position;
    vec3 position2 = vertexBufferArray[nonuniformEXT(blasIndex)].vertices[nonuniformEXT(vertexIndices[2])].position;

    vec4 albedo0 = vertexBufferArray[nonuniformEXT(blasIndex)].vertices[nonuniformEXT(vertexIndices[0])].albedo;
    vec4 albedo1 = vertexBufferArray[nonuniformEXT(blasIndex)].vertices[nonuniformEXT(vertexIndices[1])].albedo;
    vec4 albedo2 = vertexBufferArray[nonuniformEXT(blasIndex)].vertices[nonuniformEXT(vertexIndices[2])].albedo;

    vec3 normal0 = vertexBufferArray[nonuniformEXT(blasIndex)].vertices[nonuniformEXT(vertexIndices[0])].normal;
    vec3 normal1 = vertexBufferArray[nonuniformEXT(blasIndex)].vertices[nonuniformEXT(vertexIndices[1])].normal;
    vec3 normal2 = vertexBufferArray[nonuniformEXT(blasIndex)].vertices[nonuniformEXT(vertexIndices[2])].normal;

    vec2 uv0 = vertexBufferArray[nonuniformEXT(blasIndex)].vertices[nonuniformEXT(vertexIndices[0])].uv;
    vec2 uv1 = vertexBufferArray[nonuniformEXT(blasIndex)].vertices[nonuniformEXT(vertexIndices[1])].uv;
    vec2 uv2 = vertexBufferArray[nonuniformEXT(blasIndex)].vertices[nonuniformEXT(vertexIndices[2])].uv;

    vec3 positionInterpolated = interpolateVec3(position0, position1, position2, baryCoords);
    vec3 normalInterpolated = normalize(interpolateVec3(normal0, normal1, normal2, baryCoords));
    vec4 albedoInterpolated = interpolateVec4(albedo0, albedo1, albedo2, baryCoords);
    vec2 uvInterpolated = interpolateVec2(uv0, uv1, uv2, baryCoords);

    vec3 positionWorld = (gl_ObjectToWorldEXT * vec4(positionInterpolated, 1.0)).xyz;
    vec3 normalWorld = normalize((gl_WorldToObjectEXT * vec4(normalInterpolated, 0.0)).xyz); // Currently always uses smooth shading


    // =======================
    // Texture Sampling
    // =======================

    // Sample transmission from transmissionMap
    vec4 albedoSampled = texture(albedoTextures[nonuniformEXT(materialIndex)], uvInterpolated);

    // Sample emission from emissionMap
    vec4 emissionSampled = texture(emissionTextures[nonuniformEXT(materialIndex)], uvInterpolated);

    // Sample normal from normalMap
    vec4 normalSampled = texture( normalTextures[nonuniformEXT(0)],  uvInterpolated);
    normalSampled = normalSampled * 2.0 - 1.0;
    normalSampled = vec4(normalSampled[0], normalSampled[1], normalSampled[2], 0.0);
    normalSampled = normalize(normalSampled);

    // Sample roughness from roughnessMap
    float roughnessSampled = texture(roughnessTextures[nonuniformEXT(materialIndex)], uvInterpolated).r;

    // Sample metalness from metalnessMap
    float metalnessSampled = texture(metalnessTextures[nonuniformEXT(materialIndex)], uvInterpolated).r;

    // Sample transmission from transmissionMap
    float transmissionSampled = texture(transmissionTextures[nonuniformEXT(materialIndex)], uvInterpolated).r;

    vec4 albedo = albedoSampled * albedoInterpolated;

    vec3 viewDir = normalize(rayOriginWorld - positionWorld);

    vec4 accumulatedRadiance = vec4(0.0);

    // =======================
    // Lighting Calculations
    // =======================

    float lightPDF;
    for (int i = 0; i < pc.nLights; ++i) {

        // Light Properties
        vec3 lightPos =  lights[i].position;
        vec3 lightDirection = normalize(lightPos - positionWorld);
        float lightDistance = length(lightPos - positionWorld);        
        lightPDF = calculateLightPDF(lightPos, positionWorld, lights[i].radius);

        // TODO: Debug shadow ray dispatch
        // // Shadow Ray Test
        // isShadowed = false;
        // traceRayEXT(
        //     tlas,
        //     gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT,
        //     0xFF,
        //     1,                          // sbtRecordOffset = RAY_TYPE_SHADOW
        //     2,                          // sbtRecordStride = RAY_TYPE_COUNT
        //     1,                          // missIndex = RAY_TYPE_SHADOW
        //     positionWorld + normalWorld * 0.001,  // origin, offset to avoid acne
        //     0.001,                      // tmin
        //     lightDirection,
        //     lightDistance - 0.01,       // tmax
        //     1                           // payload location = 1
        // );
        // if (isShadowed) {
        //     continue; // skip this light if it is shadowed
        // }

        // BSDF Evaluation & Light Contribution
        vec4 materialResponse = evaluateBSDF(lights[i].color, albedo, normalWorld, lightDirection, viewDir);

        // Accumulate radiance
        accumulatedRadiance += materialResponse / lightPDF;

    }

    // =======================
    // Write to Payload
    // =======================

    payload.directRadiance = accumulatedRadiance;
    
    payload.albedo = albedo;
    payload.normal = vec4(normalWorld.x, normalWorld.y, normalWorld.z, 0.0);
    
    payload.albedoSampled = albedoSampled;
    payload.normalSampled = normalSampled;
    payload.emissionSampled = emissionSampled;
    payload.roughnessSampled = roughnessSampled;
    payload.metalnessSampled = metalnessSampled;
    payload.transmissionSampled = transmissionSampled;
    payload.alpha = 1.0;
    
}
