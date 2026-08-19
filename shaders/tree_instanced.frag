#version 410
#define MAX_LIGHTS 10

uniform sampler2DArray treeTextures;
uniform sampler2D specularTexture;

out vec4 fragColour;

in vec3 positionEyeSpace;
in vec3 normalEyeSpace;
in vec3 diffuseColour;
in vec2 texCoord;
in float textureIndex;

uniform vec3 specularColour;
uniform uint numLights;
uniform vec3 lightPositionsEyeSpace[MAX_LIGHTS];
uniform vec3 lightColours[MAX_LIGHTS];

const float shininess = 50.0;
const float ambientFactor = 0.2;
const float alphaCutoff = 0.01;

void main() {
    // Sample the tree texture from the correct layer based on instance texture index
    // textureIndex is passed as float but converted to int for array indexing
    vec4 treeTextureColour = texture(treeTextures, vec3(texCoord, float(int(textureIndex))));

    // Discard fully transparent fragments
    if (treeTextureColour.a < alphaCutoff) discard;

    // Modulate diffuse colour with tree texture colour
    vec3 modulatedDiffuseColour = diffuseColour * treeTextureColour.rgb;

    // Modulate specular colour with specular texture if enabled
    vec3 modulatedSpecularColour = specularColour;
    if (texture(specularTexture, texCoord).a > alphaCutoff)
    modulatedSpecularColour *= texture(specularTexture, texCoord).rgb;

    vec3 ambient, diffuse = vec3(0.0), specular = vec3(0.0);

    // Ambient component: Md * La (using 0.2 as ambient light colour)
    ambient = modulatedDiffuseColour * ambientFactor;

    vec3 unitNormalEyeSpace = normalize(normalEyeSpace);
    vec3 viewVecEyeSpace = normalize(-positionEyeSpace);
    vec3 lightVecEyeSpace, halfwayVecEyeSpace;

    for (uint lightIndex = 0; lightIndex < numLights; lightIndex++) {
        // Light vector (from vertex to light)
        lightVecEyeSpace = normalize(lightPositionsEyeSpace[lightIndex] - positionEyeSpace);

        // Diffuse component: Md * Ld * max(n.l, 0)
        diffuse += modulatedDiffuseColour * lightColours[lightIndex] * max(dot(unitNormalEyeSpace, lightVecEyeSpace), 0.0);

        // // Halfway vector for Phong-Blinn specular
        // halfwayVecEyeSpace = normalize(lightVecEyeSpace + viewVecEyeSpace);

        // // Specular component: Ms * Ls * max(n.h, 0)^f
        // specular += modulatedSpecularColour * lightColours[lightIndex] * pow(max(dot(unitNormalEyeSpace, halfwayVecEyeSpace), 0.0), shininess);
    }

    fragColour = vec4(ambient + diffuse + specular, treeTextureColour.a);
}