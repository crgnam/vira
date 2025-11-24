// =======================
// McEwan Model Fragment Shader
// =======================

#version 460
#extension GL_EXT_nonuniform_qualifier : enable
precision highp float;
precision mediump int;

// =======================
// Push Constants
// =======================
layout(push_constant) uniform PushConstants {
    uint nSpectralBands;
    uint nLights;
    uint nSamples;
} pc;

// =======================
// Inputs from the vertex shader
// =======================

layout(location = 0) in vec3 fragPosition;    // Position in view space
layout(location = 1) in vec4 fragAlbedo;      // Albedo color (RGB)
layout(location = 2) in vec3 fragNormal;      // Normal in view space

layout(location = 3) in vec2 fragUV;    // Texture coordinate
layout(location = 4) flat in uint fragInstanceID; // Instance ID
layout(location = 5) in vec4 fragNDC;


// =======================
// Outputs
// =======================
// Shader Outputs Matching Render Pass Attachments
layout(location = 0) out highp vec4 outAlbedo;       // Albedo Attachment
layout(location = 1) out highp vec4 outDirectRad;    // Direct Radiance Attachment
layout(location = 2) out highp vec4 outIndirectRad;  // Indirect Radiance Attachment
layout(location = 3) out highp vec4 outReceivedPower;        // Primary Color Attachment
layout(location = 4) out highp vec4 outNormal;       // Normals Attachment
layout(location = 5) out float outAlpha;
layout(location = 6) out uint outInstanceID;     // Instance ID Attachment
layout(location = 7) out uint outMeshID;         // Mesh ID Attachment

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
layout(set = 1, binding = 0) uniform MaterialBuffer {
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
    highp vec3 position;          // Light position
    highp float type;             // Light type identifier (0 = point, 1 = sphere)
    highp vec4 color;             // Light color/intensity
    highp float radius;           // Light radius
} lights[];


// == Descriptor Set 3: Mesh Resources ==
// Binding 0: Global Material Offset (Uniform Buffer)
layout(set = 3, binding = 0) uniform MaterialOffsetBuffer {
    uint materialOffset;
};

// Binding 1: Per Face Material Indices (Storage Buffer)
layout(set = 3, binding = 1) readonly buffer MaterialIndexBuffer {
    uint materialIndices[];
};


// == Descriptor Set 4: Debug Buffers ==
layout(set = 4, binding = 0) buffer DebugVec4 {
    vec4 debugVec4[];  // Buffer to store transformed positions
};
layout(set = 4, binding = 1) buffer DebugFloat {
    highp float  debugFloat[];  // Buffer to store transformed positions
};


// =======================
// Functions and Constants
// =======================

const float PI = 3.14159265359;

// Apply OpenCV lens distortion
vec2 distort(vec2 homogeneous, DistortionCoefficients coeff) {
    float x = homogeneous.x;
    float y = homogeneous.y;

    float r2 = x * x + y * y;
    float r4 = r2 * r2;
    float r6 = r2 * r4;

    float numerator = 1.0 + coeff.k1 * r2 + coeff.k2 * r4 + coeff.k3 * r6;
    float denominator = 1.0 + coeff.k4 * r2 + coeff.k5 * r4 + coeff.k6 * r6;
    float term1 = numerator / denominator;

    vec2 term2 = vec2(
        (2.0 * coeff.p1 * x * y) + (coeff.p2 * (r2 + 2.0 * x * x)) + (coeff.s1 * r2) + (coeff.s2 * r4),
        (coeff.p1 * (r2 + 2.0 * y * y)) + (2.0 * coeff.p2 * x * y) + (coeff.s3 * r2) + (coeff.s4 * r4)
    );

    return term1 * homogeneous + term2;
}

// Apply reverse Open CV Transformation
vec2 computeDelta(vec2 X_i, vec2 original, DistortionCoefficients coeff) {
    return distort(X_i, coeff) - original;
}

// Undistort
vec2 undistort(vec2 homogeneous, DistortionCoefficients coeff) {
    vec2 X_i = homogeneous;
    for (int i = 0; i < 20; ++i) {
        vec2 delta = computeDelta(X_i, homogeneous, coeff);
        X_i = homogeneous - delta;

        // Optional: Check for tolerance and break if convergence is reached
        if (length(delta) < 1e-6) {
            break;
        }
    }
    return X_i;
}

// Calculate the solid angle of a light
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

    // =======================
    // Initialize variables
    // =======================

    vec3 viewDir = normalize(-fragPosition);
    highp vec3 lightPos; 

    vec3  fragLightDirection;
    float fragLightDistance;

    float attenuation = 1;
    float lightPDF;
    vec4 lightColor;
    vec4 fragRadiance = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 materialResponse;

    // =======================
    // Retrieve material properties
    // =======================

    uint materialIndex = materialOffset + materialIndices[gl_PrimitiveID];
    vec4 textureAlbedo = texture(albedoTextures[nonuniformEXT(materialIndex)], fragUV);
    vec3 textureNormal = texture(normalTextures[nonuniformEXT(materialIndex)], fragUV).rbg;
    vec4 albedo = fragAlbedo;
    vec3 normal = fragNormal; // Currently always uses smooth shading

    // =======================
    // Retrieve sensor/camera properties
    // =======================

    ivec2 pixelIJ = ivec2(gl_FragCoord.xy);
    int pixelIDX = pixelIJ.x + pixelIJ.y * 1944;
    vec2 pixUV = vec2(float(pixelIJ.x)/float(1944.0), float(pixelIJ.y) /float(2592.0));
    highp float pixelSolidAngle = float(texture(pixelSolidAngleTexture, pixUV).r);

    // =======================
    // Lighting Calculations
    // =======================

    for (int i = 0; i < pc.nLights; ++i) {

        // Light Properties
        lightPos = mat3(camera.viewMatrix) * lights[i].position;
        
        fragLightDirection = normalize(lightPos - fragPosition);
        fragLightDistance = length(lightPos - fragPosition);

        lightPDF = calculateLightPDF(lightPos, fragPosition, lights[i].radius);

        materialResponse = evaluateBSDF(lights[i].color, albedo, normal, fragLightDirection, viewDir);

        // Accumulate radiance
        fragRadiance += materialResponse / lightPDF;

    }

    // =======================
    // Calculate Received Power
    // =======================

    vec4 optEff = {opticalEfficiency[0], opticalEfficiency[1], opticalEfficiency[2], opticalEfficiency[3]};
    vec4 receivedPower = camera.aperture * pixelSolidAngle * optEff * fragRadiance;
    vec4 indirectRadiance = vec4(0.0, 0.0, 0.0, 0.0); // Not Implemented Yet

    // =======================
    // Write to framebuffer attachments
    // =======================

    outAlbedo        = albedo; 
    outDirectRad     = fragRadiance;
    outIndirectRad   = indirectRadiance;
    outReceivedPower = receivedPower;
    outNormal        = vec4(normal[0], normal[1], normal[2], 0.0 ); 
    outAlpha         = 1.0;
    outInstanceID    = fragInstanceID;                    
    outMeshID        = 2u;            // Not Implemented       
    
}



