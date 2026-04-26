#version 450 core

layout(location = 0) in vec3 aLocalPos;
layout(location = 1) in vec3 aLocalNormal;

layout(location = 2) in vec4 aCenterRadius;
layout(location = 3) in vec4 aColor;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vNormal;
out vec3 vWorldPos;
out vec3 vColor;

void main() {
    vec3 center = aCenterRadius.xyz;
    float radius = aCenterRadius.w;
    vec3 worldPos = center + radius * aLocalPos;
    vWorldPos = worldPos;
    vNormal = aLocalNormal;
    vColor = aColor.rgb;
    gl_Position = uProjection * uView * vec4(worldPos, 1.0);
}
