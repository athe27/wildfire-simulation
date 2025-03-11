#version 330 core
in vec3 fragNormal;
out vec4 color;

void main() {
    float intensity = dot(normalize(fragNormal), vec3(0.0, 0.0, 1.0));  // Simple lighting
    color = vec4(intensity, intensity, intensity, 1.0);  // Output color based on lighting
}