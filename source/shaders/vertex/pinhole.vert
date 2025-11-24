// =======================
// Pinhole Model Vertex Shader
// =======================

#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_debug_printf : enable
precision highp float;

// =======================
// Vertex Buffer Attributes
// =======================
layout(location = 0) in vec3 position;   // 3 floats for position
layout(location = 1) in vec4 albedo;     // 4 floats for color (albedo)
layout(location = 2) in vec3 normal;     // 3 floats for normal
layout(location = 3) in vec2 uv;   // 2 floats for texture coordinates

// =======================
// Instance Buffer Attributes
// =======================
layout(location = 4) in mat4 modelMatrix;
layout(location = 8) in mat4 normalMatrix;  

// =======================
// Descriptors
// =======================

// == Descriptor Set 0: Camera Resources ==
struct DistortionCoefficients {
    float k1, k2, k3;  // Radial distortion coefficients
    float k4, k5, k6;  // Additional radial coefficients for denominator
    float p1, p2;      // Tangential distortion coefficients
    float s1, s2, s3, s4; // Thin prism distortion coefficients
};
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
    float k6;
    float p1;
    float p2;          
    float s1;
    float s2;
    float s3;
    float s4;        
    ivec2 resolution;
} camera;

// == Descriptor Set 4: Debug Buffers ==
// Binding 0: Debug Vec4
layout(set = 4, binding = 0) buffer DebugVec4 {
    vec4 debugVec4[];  // Buffer to store transformed positions
};
// Binding 1: Debug Float
layout(set = 4, binding = 1) buffer DebugFloat {
    float  debugFloat[];  // Buffer to store transformed positions
};

// =======================
// Outputs to Frag Shader
// =======================

layout(location = 0) out vec3 fragPosition;         // Position in view space
layout(location = 1) out vec4 fragAlbedo;           // albedo from per-vertex values in vertex buffer
layout(location = 2) out vec3 fragNormal;           // Normal in view space
layout(location = 3) out vec2 fragUV;               // Texture coordinate
layout(location = 4) flat out uint fragInstanceID;  // Instance ID
layout(location = 5) out vec4 fragNDC;

// =======================
// Main
// =======================

void main() {

    // =======================
    // Transformations
    // =======================

    // Get Transformation Matrices
    mat3 viewNormalMatrix = mat3(camera.normalMatrix);
    mat3 modelNormalMatrix = mat3(normalMatrix);     

    // Transform Normals
    vec3 worldNormal = normalize(modelNormalMatrix * normal);       // Transform normal to world space    
    vec3 viewNormal = normalize(viewNormalMatrix * worldNormal);

    // Transform Position
    vec4 modelPosition = vec4(position, 1.0);
    vec4 worldPosition = modelMatrix * modelPosition;
    vec4 viewPosition = camera.viewMatrix * worldPosition;    
    vec4 clipPosition = camera.projectionMatrix * viewPosition;
    vec4 ndcPosition = clipPosition / clipPosition.w; // Perspective division    


    // =======================
    // Set passed values 
    // =======================

    // Interpolated
    fragUV = uv;
    fragPosition = viewPosition.xyz;
    fragNormal = viewNormal;
    fragAlbedo = albedo;
    fragNDC = ndcPosition;

    // Flat
    fragInstanceID = gl_InstanceIndex;

    // Built-ins
    gl_Position = clipPosition;

}