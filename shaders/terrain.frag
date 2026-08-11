#version 410
#define MAX_LIGHTS 10

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D rockTexture;
uniform sampler2D snowTexture;
uniform sampler2D sandTexture;
uniform sampler2D waterTexture;

out vec4 fragColour;

in vec3 positionEyeSpace;
in vec3 normalEyeSpace;
in vec3 diffuseColour;
in vec2 texCoord;
in float slope;
in float heightNorm;
in float waterLevelNorm;

uniform vec3 specularColour;
uniform bool applyDiffuseTexture;
uniform bool applySpecularTexture;
uniform uint numLights;
uniform vec3 lightPositionsEyeSpace[MAX_LIGHTS];
uniform vec3 lightColours[MAX_LIGHTS];

const float shininess = 50.0;
const float ambientFactor = 0.2;
const float alphaCutoff = 0.01;

void main() {
    // Texture colors
    vec3 grassColour = texture(diffuseTexture, texCoord).rgb;
    vec3 rockColour = texture(rockTexture, texCoord).rgb;
    vec3 snowColour = texture(snowTexture, texCoord).rgb;
    vec3 sandColour = texture(sandTexture, texCoord).rgb;
    vec3 waterColour = texture(waterTexture, texCoord * 2.0).rgb;

    // Terrain coloring with height and slope blending
    float heightBlend = smoothstep(0.55, 0.75, heightNorm);
    float slopeBlend = smoothstep(0.25, 0.5, slope);

    vec3 baseColour = mix(grassColour, snowColour, heightBlend);
    vec3 terrainColour = mix(baseColour, rockColour, slopeBlend);

    // Sand is blended out on steep slopes (cliffs) and only appears in the beach zone
    float flatness = 1.0 - slope;  // 1.0 = flat, 0.0 = steep/cliff
    float sandThreshold = waterLevelNorm + 0.02;  // Sand appears above water but stops at this height
    float sandHeightBlend = smoothstep(waterLevelNorm, sandThreshold, heightNorm);

    // Multiply by flatness to hide sand on steep terrain
    float sandBlend = sandHeightBlend * flatness;
    vec3 sandLayer = mix(sandColour, terrainColour, sandBlend);

    //blend toward a deep blue as depth increases
    vec3 modulatedDiffuseColour;
    if (heightNorm <= waterLevelNorm) {
        float depth = waterLevelNorm - heightNorm;

        // Adjust the divisor (0.15) to control how quickly the transition happens.
        vec3 deepColour = vec3(0.0, 0.05, 0.2);
        float blend = clamp(depth / 0.2, 0.0, 1.0);
        vec3 shadedWater = mix(waterColour * diffuseColour * 3.0, deepColour, blend);
        modulatedDiffuseColour = shadedWater;
    } else {
        modulatedDiffuseColour = sandLayer * diffuseColour;
    }

    float alpha = texture(diffuseTexture, texCoord).a;
    //if (alpha < alphaCutoff) discard; // This provides limited transparency support even when blending is disabled

    // Modulate specular colour with specular texture if enabled
    vec3 modulatedSpecularColour = specularColour;
    if (applySpecularTexture) modulatedSpecularColour *= texture(specularTexture, texCoord).rgb;


    vec3 ambient, diffuse = vec3(0.0), specular = vec3(0.0);
    // Calculate ambient component of lighting: Ma*La. Here we just use the diffuse colour (Md) as Ma, and 0.2 in place of light ambient colour La
    ambient = modulatedDiffuseColour * ambientFactor;

    vec3 unitNormalEyeSpace = normalize(normalEyeSpace);
    vec3 viewVecEyeSpace = normalize(-positionEyeSpace);
    vec3 lightVecEyeSpace, halfwayVecEyeSpace;
    for (uint lightIndex = 0; lightIndex < numLights; lightIndex++) {
        // Calculate l (unit vector pointing towards the light source from the vertex)
        lightVecEyeSpace = normalize(lightPositionsEyeSpace[lightIndex] - positionEyeSpace);

        // Calculate diffuse component of lighting: Md*Ld*max(n.l, 0)
        diffuse += modulatedDiffuseColour * lightColours[lightIndex] * max(dot(unitNormalEyeSpace, lightVecEyeSpace), 0.0);

        // Calculate h (the halfway vector, l+v)
        halfwayVecEyeSpace = normalize(lightVecEyeSpace + viewVecEyeSpace);
        // Calculate specular component of lighting: Ms*Ls*max(n.h, 0)**f
        if (heightNorm < waterLevelNorm) specular += modulatedSpecularColour * lightColours[lightIndex] * pow(max(dot(unitNormalEyeSpace, halfwayVecEyeSpace), 0.0), shininess);
    }

    fragColour = vec4(ambient + diffuse + specular, alpha);
}
