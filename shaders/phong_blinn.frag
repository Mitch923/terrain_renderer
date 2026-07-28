#version 410
#define MAX_LIGHTS 10

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

out vec4 fragColour;

in vec3 positionEyeSpace;
in vec3 normalEyeSpace;
in vec3 diffuseColour;
in vec2 texCoord;

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
    // Modulate diffuse colour with diffuse texture if enabled
    vec3 modulatedDiffuseColour = diffuseColour;
    float alpha = 1.0;
    if (applyDiffuseTexture) {
        vec4 diffuseTextureColour = texture(diffuseTexture, texCoord);
        // The alpha channel denotes opacity. We will record it to use in the final fragColour, but blending needs to be enabled for it to have an effect
        alpha = diffuseTextureColour.a;
        if (alpha < alphaCutoff) discard; // This provides limited transparency support even when blending is disabled
        modulatedDiffuseColour *= diffuseTextureColour.rgb;
    }

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
        specular += modulatedSpecularColour * lightColours[lightIndex] * pow(max(dot(unitNormalEyeSpace, halfwayVecEyeSpace), 0.0), shininess);
    }

    fragColour = vec4(ambient + diffuse + specular, alpha);
}