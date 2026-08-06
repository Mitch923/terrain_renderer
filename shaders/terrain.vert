#version 410

layout (location = 0) in vec3 position;
uniform vec3 terrainOffset; // Shifted each frame so the grid follows the camera

void main() {
    gl_Position = vec4(position + terrainOffset, 1.0);
}
