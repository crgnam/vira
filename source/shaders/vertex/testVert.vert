#version 460

// Vertex buffer (binding = 0)
layout(location = 0) in vec3 inPosition;     // Vertex position
layout(location = 1) in vec3 inAlbedo;       // Vertex color or albedo
layout(location = 2) in vec3 inNormal;       // Vertex normal
layout(location = 3) in vec2 inUV;           // Texture coordinates

// Instance buffer (binding = 1)
// First mat4 
layout(location = 4) in vec4 inModel0;
layout(location = 5) in vec4 inModel1;
layout(location = 6) in vec4 inModel2;
layout(location = 7) in vec4 inModel3;

// Second mat4 
layout(location = 8) in vec4 inExtra0;
layout(location = 9) in vec4 inExtra1;
layout(location = 10) in vec4 inExtra2;
layout(location = 11) in vec4 inExtra3;

layout(location = 0) out vec3 fragColor;

void main() {
    fragColor = inAlbedo;
    gl_Position = vec4(inPosition, 1.0);
}
