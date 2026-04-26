#version 450 core

layout(location = 0) in vec3 aLocalPos;
layout(location = 1) in vec3 aLocalNormal;

layout(location = 2) in mat4 aModel;
layout(location = 6) in vec4 aColor;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vNormal;
out vec3 vWorldPos;
out vec3 vColor;

void main() {
    vec4 worldPos = aModel * vec4(aLocalPos, 1.0);
    vWorldPos = worldPos.xyz;
    mat3 normalMat = mat3(transpose(inverse(aModel)));
    vNormal = normalize(normalMat * aLocalNormal);
    vColor = aColor.rgb;
    gl_Position = uProjection * uView * worldPos;
}
