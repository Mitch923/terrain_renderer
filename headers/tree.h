#ifndef TREE_H
#define TREE_H

#include "mesh.h"
#include "texture.h"
#include "window.h"
#include "terrain_noise.h"
#include "shader.h"
#include "tree_texture_array.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <limits>
#include <cstdint>

void drawMesh(ShaderProgram &program, Mesh &mesh, glm::mat4 &view);

// Extern declarations for globals defined in main.cpp
extern uint32_t terrainSeed;
extern const float TERRAIN_HEIGHT_BOUND;
extern const float TERRAIN_FREQ;
extern const float TERRAIN_AMP;
extern const float WATER_LEVEL;

namespace tree {
    // Tree settings
    constexpr float TREE_QUAD_SIZE = 2.0f; // Half-extent of each cross-billboard face (world units)
    constexpr int NUM_TREE_TEXTURES = 3; // Crabapple / MapleFall / Pine
    constexpr unsigned int MAX_TREES = 4096; // Upper bound on tree count across all textures

    // Tree density is fraction of valid-grid cells that become a tree. Larger = denser forest.
    const float TREE_DENSITY = 0.75f;
    constexpr float MIN_TREE_SEPARATION = 3.0f; // Spacing between candidate spawn positions
    constexpr float TREE_HEIGHT_OFFSET = -0.1f; // Lift trees above ground by this amount
    constexpr float TREE_SNOW_HEIGHT_NORM = 0.55f; // Must match the start of snow blending in terrain.frag
    constexpr float TREE_CLIFF_SLOPE = 0.25f; // Must match the start of rock blending in terrain.frag

    // Per-texture density multipliers, allows sparse snow/desert variants later if desired.
    const float TREE_DENSITY_PER_TEXTURE[NUM_TREE_TEXTURES] = {TREE_DENSITY, TREE_DENSITY, TREE_DENSITY};

    constexpr const char *TREE_TEXTURE_NAMES[NUM_TREE_TEXTURES] = {
        "tree_crabapple", "tree_maplefall", "tree_pine"
    };


    struct Placement {
        glm::vec3 worldPos;
        int textureIndex;
    };

    std::vector<Placement> treePlacements;

    glm::ivec2 generatedCell{std::numeric_limits<int>::min(), std::numeric_limits<int>::min()};

    constexpr GLsizei INSTANCE_STRIDE = 4 * sizeof(float);

    GLuint instanceVBO = 0;

    // Texture array for all tree textures
    TreeTextureArray treeTextureArray;

    // Upload current treePlacements into the instance VBO
    void updateInstanceBuffer() {
        if (treePlacements.empty()) {
            std::cerr << "Warning: No tree placements to upload to instance buffer." << std::endl;
            return;
        }

        // Build CPU-side instance data: interleaved vec3 position + int textureIndex
        std::vector<float> instanceData(treePlacements.size() * 4);
        for (size_t i = 0; i < treePlacements.size(); i++) {
            instanceData[i * 4 + 0] = treePlacements[i].worldPos.x;
            instanceData[i * 4 + 1] = treePlacements[i].worldPos.y;
            instanceData[i * 4 + 2] = treePlacements[i].worldPos.z;
            instanceData[i * 4 + 3] = static_cast<float>(treePlacements[i].textureIndex);
        }

        // Create or update the instance VBO
        if (instanceVBO == 0) {
            glGenBuffers(1, &instanceVBO);
        }
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(instanceData.size() * sizeof(float)),
                     instanceData.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    //  Hash same as GPU
    uint32_t treeHash(int x, int z, uint32_t salt = 0u) {
        uint32_t h = static_cast<uint32_t>(x) * 0x8da6b343u
                     + static_cast<uint32_t>(z) * 0xd8163841u
                     + terrainSeed + salt;
        h ^= h >> 16u;
        h *= 0x7feb352du;
        h ^= h >> 15u;
        h *= 0x846ca68bu;
        h ^= h >> 16u;
        return h;
    }

    float treeRandom(int x, int z, uint32_t salt = 0u) {
        return static_cast<float>(treeHash(x, z, salt)) /
               static_cast<float>(std::numeric_limits<uint32_t>::max());
    }

    bool canPlaceTree(glm::vec2 position, float height, uint32_t terrainSeed) {
        if (height <= WATER_LEVEL) return false;

        const float heightNorm = height / TERRAIN_HEIGHT_BOUND * 0.5f + 0.5f;
        if (heightNorm >= TREE_SNOW_HEIGHT_NORM) return false;

        // Estimate the normal exactly as terrain.eval does for non-water terrain.
        constexpr float normalSampleDistance = 0.5f;
        const float hL = terrain_noise::getTerrainHeight(position - glm::vec2(normalSampleDistance, 0.0f), terrainSeed,
                                                         TERRAIN_HEIGHT_BOUND, TERRAIN_FREQ, TERRAIN_AMP);
        const float hR = terrain_noise::getTerrainHeight(position + glm::vec2(normalSampleDistance, 0.0f), terrainSeed,
                                                         TERRAIN_HEIGHT_BOUND, TERRAIN_FREQ, TERRAIN_AMP);
        const float hD = terrain_noise::getTerrainHeight(position - glm::vec2(0.0f, normalSampleDistance), terrainSeed,
                                                         TERRAIN_HEIGHT_BOUND, TERRAIN_FREQ, TERRAIN_AMP);
        const float hU = terrain_noise::getTerrainHeight(position + glm::vec2(0.0f, normalSampleDistance), terrainSeed,
                                                         TERRAIN_HEIGHT_BOUND, TERRAIN_FREQ, TERRAIN_AMP);
        const glm::vec3 normal = glm::normalize(glm::cross(
            glm::vec3(0.0f, hU - hD, 2.0f * normalSampleDistance),
            glm::vec3(2.0f * normalSampleDistance, hR - hL, 0.0f)));
        return 1.0f - normal.y < TREE_CLIFF_SLOPE;
    }

    // updates current tree placements
    // returns true if at least one tree was updated
    bool updatePlacements(float renderDistance, uint32_t terrainSeed) {
        const glm::ivec2 cameraCell = glm::floor(glm::vec2(window::eyePos.x, window::eyePos.z) /
                                                 MIN_TREE_SEPARATION);
        if (cameraCell == generatedCell) return false;
        generatedCell = cameraCell;
        treePlacements.clear();

        const int cellsFromCamera = static_cast<int>(std::ceil(renderDistance / MIN_TREE_SEPARATION));
        for (int z = cameraCell.y - cellsFromCamera; z <= cameraCell.y + cellsFromCamera; ++z) {
            for (int x = cameraCell.x - cellsFromCamera; x <= cameraCell.x + cellsFromCamera; ++x) {
                if (treePlacements.size() >= MAX_TREES) return x > 0 || z > 0; // At least one tree has been updated

                const int textureIndex = static_cast<int>(treeHash(x, z, 1u) % NUM_TREE_TEXTURES);
                if (treeRandom(x, z, 2u) > TREE_DENSITY_PER_TEXTURE[textureIndex]) continue;

                // Jitter within each cell avoids an obvious square planting pattern.
                const glm::vec2 position = (glm::vec2(x, z) + glm::vec2(
                                                treeRandom(x, z, 3u) - 0.5f,
                                                treeRandom(x, z, 4u) - 0.5f)) * MIN_TREE_SEPARATION;
                const float height = terrain_noise::getTerrainHeight(position, terrainSeed,
                                                                     TERRAIN_HEIGHT_BOUND, TERRAIN_FREQ, TERRAIN_AMP);
                if (!canPlaceTree(position, height, terrainSeed)) continue;

                // The billboard is modelled around its centre, so put its bottom at the terrain.
                treePlacements.push_back({
                    glm::vec3(position.x, height + TREE_QUAD_SIZE * 1.5f + TREE_HEIGHT_OFFSET, position.y),
                    textureIndex
                });
            }
        }

        return true;
    }

    // Instanced drawing
    void drawTrees(glm::mat4 &view, ShaderProgram &program, Mesh &billboard,
                   const TreeTextureArray &texArray) {
        if (treePlacements.empty()) return;

        // For billboards
        glDisable(GL_CULL_FACE);

        // Set model/normal matrices to identity (instance positions come from the VBO)
        glm::mat4 identity = glm::mat4(1.0f);
        glUniformMatrix4fv(program.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(identity));
        glUniformMatrix4fv(program.getUniformLocation("normalMatrix"), 1, GL_TRUE, glm::value_ptr(identity));
        glUniform3fv(program.getUniformLocation("specularColour"), 1, glm::value_ptr(glm::vec3(1.0f)));
        glUniform1i(program.getUniformLocation("applyDiffuseTexture"), GL_FALSE); // not used — texture is in the array
        glUniform1i(program.getUniformLocation("applySpecularTexture"), GL_FALSE);

        // Bind the texture array to texture unit 0 and set the sampler uniform
        texArray.bind(0);
        program.setAsActiveProgram();
        glUniform1i(program.getUniformLocation("treeTextures"), 0);

        // Bind the billboard VAO (source per-vertex attributes)
        glBindVertexArray(billboard.vaoID);

        const GLsizei quadIndexCount = 6; // 2 tris per quad, from makeCrossBillboardTree's element order
        const GLsizei instanceCount = static_cast<GLsizei>(treePlacements.size());

        // Single instanced draw call
        glDrawElementsInstanced(GL_TRIANGLES,
                                static_cast<GLsizei>(billboard.elements.size()),
                                GL_UNSIGNED_INT,
                                NULL,
                                static_cast<GLsizei>(treePlacements.size()));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        glEnable(GL_CULL_FACE);
    }

    void init(std::unordered_map<std::string, Texture> &textures, Mesh &billboard) {
        billboard = basic_objects::makeCrossBillboardTree(TREE_QUAD_SIZE, glm::vec3(1.0f));

        // Setup VBO
        glGenBuffers(1, &instanceVBO);
        glBindVertexArray(billboard.vaoID);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

        // Attribute location 4: instance world position (vec3)
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, INSTANCE_STRIDE, (void *) 0);
        glEnableVertexAttribArray(4);
        glVertexAttribDivisor(4, 1); // per-instance

        // Attribute location 5: instance texture index (float — converted to int in shader)
        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, INSTANCE_STRIDE, (void *) (3 * sizeof(float)));
        glEnableVertexAttribArray(5);
        glVertexAttribDivisor(5, 1); // per-instance

        // Build the texture array from the three tree texture files
        std::vector<std::string> treeTexturePaths = {
            "../assets/tree_textures/Crabapple_baseColor.png",
            "../assets/tree_textures/MapleFall_baseColor.png",
            "../assets/tree_textures/Pine_baseColor.png"
        };
        treeTextureArray.createFromArray(treeTexturePaths, GL_CLAMP_TO_EDGE);
        if (treeTextureArray.textureID == 0) {
            std::cerr << "Error: Failed to create tree texture array." << std::endl;
        }

        // Also load individual textures for the lookup map (kept for compatibility)
        textures["tree_crabapple"] = Texture("../assets/tree_textures/Crabapple_baseColor.png", GL_CLAMP_TO_EDGE, true,
                                             false);
        textures["tree_maplefall"] = Texture("../assets/tree_textures/MapleFall_baseColor.png", GL_CLAMP_TO_EDGE, true,
                                             false);
        textures["tree_pine"] = Texture("../assets/tree_textures/Pine_baseColor.png", GL_CLAMP_TO_EDGE, true, false);
    }
} // namespace tree

#endif // TREE_H
