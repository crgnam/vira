#version 460

layout(location = 0) in vec3 fragColor;


layout(location = 0) out vec4 outColor0;
layout(location = 1) out vec4 outColor1;
layout(location = 2) out vec4 outColor2;
layout(location = 3) out vec4 outColor3;
layout(location = 4) out vec4 outColor4;
layout(location = 5) out vec4 outColor5;
layout(location = 6) out vec4 outColor6;
layout(location = 7) out vec4 outColor7;

void main() {
    vec4 finalColor = vec4(fragColor, 1.0);
    outColor0 = finalColor;
    outColor1 = finalColor;
    outColor2 = finalColor;
    outColor3 = finalColor;
    outColor4 = finalColor;
    outColor5 = finalColor;
    outColor6 = finalColor;
    outColor7 = finalColor;
}
