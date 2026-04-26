#version 450 core

in vec3 vNormal;
in vec3 vWorldPos;
in vec3 vColor;

uniform vec3 uLightPos;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vWorldPos);
    float ndl = max(dot(N, L), 0.0);
    float ambient = 0.18;
    vec3 lit = vColor * (ambient + (1.0 - ambient) * ndl);
    FragColor = vec4(lit, 1.0);
}
