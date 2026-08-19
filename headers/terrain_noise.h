#ifndef TERRAIN_NOISE_H
#define TERRAIN_NOISE_H

#include <glm/glm.hpp>
#include <cstdint>
#include <cmath>

// Extracted from shaders/terrain.eval for consistent terrain classification between GPU and CPU

namespace terrain_noise {

    const float PI = 3.14159265358979f;

    // Gradient hash function - identical to GLSL version
    glm::vec2 gradientHash(glm::ivec2 pos, uint32_t seed) {
        uint32_t h = (uint32_t)pos.x * 646016809u + (uint32_t)pos.y * 869240873u + seed * 406460479u;

        // Triple 32 scrambler from shadertoy
        h ^= h >> 17u;
        h *= 0xed5ad4bbu;
        h ^= h >> 11u;
        h *= 0xac4c1b51u;
        h ^= h >> 15u;
        h *= 0x31848babu;
        h ^= h >> 14u;

        // Convert to angle and return gradient vector
        float angle = (float(h) / float(0xffffffffu)) * 2.0f * PI;
        return glm::vec2(cos(angle), sin(angle));
    }

    // Smootherstep interpolation - identical to GLSL version
    glm::vec2 smootherStep(glm::vec2 f) {
        return f * f * f * (f * (f * 6.0f - 15.0f) + 10.0f);
    }

    // 2D Perlin noise with gradient hashing
    float perlinNoise(glm::vec2 coord, uint32_t seed) {
        glm::ivec2 i = glm::ivec2(floor(coord));
        glm::vec2 f = fract(coord);

        // Random gradients for each corner
        glm::vec2 g00 = gradientHash(i + glm::ivec2(0, 0), seed);
        glm::vec2 g01 = gradientHash(i + glm::ivec2(0, 1), seed);
        glm::vec2 g10 = gradientHash(i + glm::ivec2(1, 0), seed);
        glm::vec2 g11 = gradientHash(i + glm::ivec2(1, 1), seed);

        // Vectors from corners
        glm::vec2 v00 = f - glm::vec2(0.0f, 0.0f);
        glm::vec2 v01 = f - glm::vec2(0.0f, 1.0f);
        glm::vec2 v10 = f - glm::vec2(1.0f, 0.0f);
        glm::vec2 v11 = f - glm::vec2(1.0f, 1.0f);

        // Dot products for corners and local vectors
        float d00 = dot(g00, v00);
        float d01 = dot(g01, v01);
        float d10 = dot(g10, v10);
        float d11 = dot(g11, v11);

        glm::vec2 u = smootherStep(f);

        // Bilinear interpolation
        float nx0 = glm::mix(d00, d10, u.x);
        float nx1 = glm::mix(d01, d11, u.x);
        float noiseVal = glm::mix(nx0, nx1, u.y);

        // Normalize to [-1, 1] range (sqrt(2) normalization factor)
        return noiseVal * 1.41421356237f;
    }

    // Multi-octave terrain height function - identical to GLSL version
    float getTerrainHeight(glm::vec2 pos, uint32_t seed, float terrainHeightBound, float terrainFrequency, float terrainAmplitude) {
        float totalHeight = 0.0f;
        float frequency = terrainFrequency;
        float amplitude = terrainAmplitude;
        float maxAmplitude = 0.0f;

        for (int i = 0; i < 5; i++) {
            float noise = perlinNoise(pos * frequency, seed);
            totalHeight += noise * amplitude;
            maxAmplitude += amplitude;

            amplitude *= 0.5f;
            frequency *= 2.0f;
        }

        return (totalHeight / maxAmplitude) * terrainHeightBound;
    }

} // namespace terrain_noise

#endif // TERRAIN_NOISE_H
