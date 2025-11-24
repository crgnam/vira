// =======================
// Ray Generation Shader
// =======================

#version 460 core
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable


// =======================
// Constants
// =======================
uint seed = 12345u; // Initialize a global seed
float rand() {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return float(seed & 0x7FFFFFFFu) / float(0x7FFFFFFF);
}
const float PI = 3.14159265359;
const float VeryLarge = 1e10;

// =======================
// Payload 
// =======================
struct DataPayload {

    vec4 receivedPower;         
    vec4 albedo;                
    vec4 directRadiance;        
    vec4 indirectRadiance;      
    vec4 normal;                
    vec2 uvInterpolated;
    float depth;               
    uint samples;              
    uint instanceID;           
    uint meshID;               
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
layout(location = 0) rayPayloadEXT DataPayload payload;

// =======================
// Push Constants 
// =======================
layout(push_constant) uniform PushConstants {
    uint nSpectralBands;
    uint nLights;
    uint nSamples;
} pc;

// =======================
// Descriptors
// =======================

// == Descriptor Set 0: Camera ==
struct DistortionCoefficients {
    float k1, k2, k3;       // Radial distortion coefficients
    float k4, k5, k6;       // Additional radial coefficients for denominator
    float p1, p2;           // Tangential distortion coefficients
    float s1, s2, s3, s4;   // Thin prism distortion coefficients
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

// Binding 2
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

// Binding 2: Per Face materials
layout(set = 4, binding = 2) buffer FaceMaterialBuffers {
    int materialIDs[];
} faceMaterialIDArray[];

// Bidning 3: Material Offsets
layout(set = 4, binding = 3) uniform MaterialOffsets {
    uint materialOffsets[];
};


// == Descriptor Set 5: Image Resources (Direct Radiance, Color, Output Image) ==

// Binding 0: Multi-Channel Image for One Spectral Set
// Layer 0: albedoImage;
// Layer 1: directRadianceImage;
// Layer 2: indirectRadianceImage;
// Layer 3: receivedPowerImage;
// Layer 4: backgroundPowerImage;
// Layer 5: normalImage (padded);
layout(set = 5, binding = 0, rgba32f) uniform writeonly image2DArray spectralImages[];
       
// Binding 1: Multi-layered Scalar Image 
// Layer 0:  depthImage;
// Layer 1:  alphaImage;
// Layer 2:  triangleSizeImage;
// Layer 3: meshIDImage;
// Layer 4: instanceIDImage;
// Layer 5: triangleIDImage;
layout(set = 5, binding = 1, r32f) uniform writeonly image2DArray scalarImages;
    

// == Descriptor Set 6: Debug Buffers ==
// Binding 0: Ray Generation Shader Invocation Counter
layout(set = 6, binding = 0) buffer DebugCounter {
    uint debugCounterRay;
};
// Binding 3: Debug Vector
layout(set = 6, binding = 3) buffer DebugVector {
    vec4 debugVec4[];
};

// =======================
// Functions
// =======================

vec2 distort(vec2 homogeneous) {

    // DistortionCoefficients d = camera.distortion;
    float x = homogeneous.x;
    float y = homogeneous.y;

    float r2 = x * x + y * y;
    float r4 = r2 * r2;
    float r6 = r4 * r2;

    float numerator = 1 + camera.k1 * r2 + camera.k2 * r4 + camera.k3 * r6;
    float denominator = 1 + camera.k4 * r2 + camera.k5 * r4 + camera.k6 * r6;

    float term1 = numerator / denominator;

    vec2 term2 = vec2(
            (2 * camera.p1 * x * y) + (camera.p2 * (r2 + 2 * x * x)) + (camera.s1 * r2) + (camera.s2 * r4),
            (camera.p1* (r2 + 2 * y * y)) + (2 * camera.p2 * x * y) + (camera.s3 * r2) + (camera.s4 * r4)
    );

    return (term1 * homogeneous) + term2;
}

vec2 computeDelta(vec2 homogeneous) {
    return (homogeneous - distort(homogeneous));
}

vec2 undistort(vec2 homogeneous) {
    vec2 X_i = vec2(homogeneous.x, homogeneous.y);
    for (int i = 0; i < 20; ++i) {
        vec2 delta = computeDelta(X_i);
        X_i = homogeneous - delta;

        // // Optional: Check for tolerance and break if convergence is reached
        // if (length(delta) < 1e-6) {
        //     break;
        // }
    }
    return X_i;
}

mat3x2 trimMat4ToMat3x2(mat4 m) {
    return mat3x2(
        m[0].xy, // First column (first 3 rows)
        m[1].xy,  // Second column (first 3 rows)
        m[2].xy
    );
}


// =======================
// Main
// =======================
void main() {

    // =======================
    // Debug 
    // =======================
    memoryBarrierBuffer();
    atomicAdd(debugCounterRay, 1);
    memoryBarrierBuffer();  

    // =======================
    // Initialize
    // =======================

    // Pixel coordinates and normalized device coordinates (NDC)
    ivec2 pixelIndices = ivec2(gl_LaunchIDEXT.xy); // lower left corner of pixel
    int pixelIndex = pixelIndices.y * camera.resolution.x + pixelIndices.x;

    // Initialize ray payload
    payload.pixelIndex  = pixelIndex;

    vec4 accumulatedRadiance = vec4(0.0);
    vec4 accumulatedAlbedo = vec4(0.0);
    float accumulatedAlpha = 0.0;

    // =======================
    // Sample rays in pixel
    // =======================
    for (int i = 0; i < pc.nSamples; ++i) {        

        payload.directRadiance = vec4(0.0);
        payload.albedo = vec4(0.0);
        payload.alpha = 0.0;
        
        // Sample Pixel
        float di = rand();
        float dj = rand();
        vec2 pixelCoords = vec2(float(pixelIndices.x) + di, float(pixelIndices.y) + dj );

        // =======================
        // Construct Ray
        // =======================
        
        // Get pixel homogeneous coordinates
        mat3x2 Kinv = trimMat4ToMat3x2(camera.inverseIntrinsicMatrix);
        vec2 rayHomogeneous = Kinv * vec3(pixelCoords.x, pixelCoords.y, 1.0);

        // Undistort homogenous coordinates
        vec2 rayUndistortedHomogeneous = undistort(rayHomogeneous);

        // Construct ray in view frame
        vec3 rayView = vec3(rayUndistortedHomogeneous[0], camera.zDir * rayUndistortedHomogeneous[1], camera.zDir);

        // Transform the ray direction to world space, making sure to set the homogeneous w coordinate to 0 as we are transforming a direction
        vec3 rayWorld = vec3(camera.inverseViewMatrix * vec4(rayView, 0.0)); // 

        // Create ray direction and origin
        vec3 rayDirection = normalize(rayWorld);
        vec3 rayOrigin = camera.cameraPosition;  // Camera's world-space position

        // Apply depth-of-field
        if (camera.depthOfField) {

            // Sample the aperture
            float r = sqrt(rand());
            float theta = 2 * PI * rand();
            vec2 aperturePoint = vec2( r * cos(theta), r * sin(theta) );
            vec3 originOffset = vec3( aperturePoint[0], aperturePoint[1], camera.focalLength);

            // Transform the offset to world space and add to the ray origin
            mat3 inverseRotationMatrix = mat3(camera.inverseViewMatrix);
            rayOrigin += inverseRotationMatrix * originOffset;

            // Adjust ray direction if focus distance isn't infinite
            if (camera.focusDistance < VeryLarge) {
                rayDirection = camera.focusDistance * rayDirection - originOffset;
            }
        }

        float tmin = 0.0;
        float tmax = 1.0e32;

        // =======================
        // Trace the ray
        // =======================

        uint sbtRecordOffset = 0; 
        uint sbtRecordStride = 0; 
        uint missIndex = 0; 
        traceRayEXT(
            tlas,                       // Acceleration structure
            gl_RayFlagsOpaqueEXT,       // Ray flags (opaque geometry)
            0xFF,                       // Cull mask
            0,                          // SBT record offset for ray type 0
            2,                          // SBT record stride
            0,                          // Miss shader index
            rayOrigin,                  // Ray origin
            tmin,                       // Minimum ray distance (avoid self-intersections)
            rayDirection,               // Ray direction
            tmax,                       // Maximum ray distance (avoid self-intersections)
            0 );                        // payload location

        // Accumulate sample contribution
        accumulatedRadiance += payload.directRadiance;
        accumulatedAlbedo   += payload.albedo;
        accumulatedAlpha    += payload.alpha;
    }


    // =======================
    // Get mean photometric values from rays
    // =======================

    accumulatedRadiance = accumulatedRadiance / pc.nSamples;
    accumulatedAlbedo   = accumulatedAlbedo / pc.nSamples;
    float alpha = accumulatedAlpha / pc.nSamples;


    // =======================
    // Calculate Received Power    
    // =======================

    float pixelSolidAngle = float(texture(  pixelSolidAngleTexture, 
                                            vec2(float(pixelIndices[0]), float(pixelIndices[1]))
                                        ).r);
    vec4 optEff = {opticalEfficiency[0], opticalEfficiency[1], opticalEfficiency[2], opticalEfficiency[3]};
    
    vec4 receivedPower = camera.aperture * pixelSolidAngle * optEff * accumulatedRadiance;


    // =======================
    // Write Data to Image Buffers
    // =======================

    // Spectral Image
    // Layer 0: albedoImage;
    imageStore(spectralImages[0], ivec3(pixelIndices, 0), accumulatedAlbedo);

    // Layer 1: directRadianceImage;
    imageStore(spectralImages[0], ivec3(pixelIndices, 1), accumulatedRadiance);

    // Layer 2: indirectRadianceImage;
    imageStore(spectralImages[0], ivec3(pixelIndices, 2), vec4(0.0, 0.0, 0.0, 0.0));//payload.indirectRadiance);

    // Layer 3: receivedPowerImage;
    imageStore(spectralImages[0], ivec3(pixelIndices, 3), receivedPower);

    // Layer 4: backgroundPowerImage;
    imageStore(spectralImages[0], ivec3(pixelIndices, 4), vec4(0.0, 0.0, 0.0, 0.0));

    // Layer 5: normalImage (padded);
    imageStore(spectralImages[0], ivec3(pixelIndices, 5), payload.normal);

    // Layer 0:  alphaImage;
    imageStore(scalarImages, ivec3(pixelIndices, 0), vec4(alpha, 0.0, 0.0, 0.0));

    // Scalar Image
    // Layer 1:  depthImage;
    imageStore(scalarImages, ivec3(pixelIndices, 1), vec4(payload.depth, 0, 0, 0));
   
    // Layer 2:  triangleSizeImage;
    
    // Layer 3: meshIDImage;
    imageStore(scalarImages, ivec3(pixelIndices, 3), vec4(payload.meshID, 0, 0, 0));
    
    // Layer 4: instanceIDImage;
    imageStore(scalarImages, ivec3(pixelIndices, 4), vec4(payload.instanceID, 0, 0, 0));
    
    // Layer 5: triangleIDImage;
    
    //Debug Output
    debugVec4[nonuniformEXT(pixelIndex)] = vec4(0.0, 0.0, 0.0, 0.0);

    return;
}
