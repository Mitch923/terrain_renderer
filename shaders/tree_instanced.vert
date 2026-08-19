#version 410

layout (location = 0) in vec3 positionIn;
layout (location = 1) in vec3 normalIn;
layout (location = 2) in vec3 colourIn;
layout (location = 3) in vec2 texCoordIn;

// Per-instance attributes (fetched via gl_InstanceID)
layout (location = 4) in vec3 instanceWorldPos;
layout (location = 5) in float instanceTextureIndex;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 normalMatrix;

out vec3 positionEyeSpace;
out vec3 normalEyeSpace;
out vec3 diffuseColour;
out vec2 texCoord;
out float textureIndex;

void main() {
    // Transform vertex position by adding the per-instance world offset
    vec3 worldPos = vec3(modelMatrix * vec4(positionIn, 1.0)) + instanceWorldPos;

    // Calculate eye space coordinates of the vertex
    positionEyeSpace = vec3(viewMatrix * vec4(worldPos, 1.0));

    // Calculate eye space normal using inverse transpose model-view matrix
    normalEyeSpace = normalize(vec3(normalMatrix * vec4(normalIn, 1.0)));

    // Project to clip space
    gl_Position = projMatrix * vec4(positionEyeSpace, 1.0);

    diffuseColour = colourIn;
    texCoord = texCoordIn;
    textureIndex = instanceTextureIndex;
}