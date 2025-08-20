#version 450 core
out vec4 FragColor;
in vec3 Normal, FragPos;
uniform vec3 lightPos, atomColor;
void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * atomColor;
    FragColor = vec4(diffuse, 1.0);
} 