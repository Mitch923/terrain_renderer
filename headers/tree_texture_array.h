/* COSC422 Advanced Computer Graphics (2026)

tree_texture_array.h: Provides a TreeTextureArray class that packs multiple
tree textures into a single GL_TEXTURE_2D_ARRAY for instanced rendering.
This eliminates per-draw-call texture binding overhead.
*/

#ifndef TREE_TEXTURE_ARRAY_H
#define TREE_TEXTURE_ARRAY_H

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/freeglut.h>
#include <vector>
#include <string>
#include <iostream>

#include "texture.h"
#include "shader.h"

class TreeTextureArray {
public:
    GLuint textureID;
    GLint uniformLocation;

    TreeTextureArray() : textureID(0), uniformLocation(-1) {
    }

    // Create a 2D texture array from a list of texture files
    void createFromArray(const std::vector<std::string> &texturePaths, GLuint wrapMode = GL_CLAMP_TO_EDGE) {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);

        GLint maxArrayLayers;
        glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxArrayLayers);
        if (maxArrayLayers < (int) texturePaths.size()) {
            std::cerr << "Warning: GPU supports only " << maxArrayLayers
                    << " 2D array layers, but " << texturePaths.size() << " were requested." << std::endl;
        }

        int texWidth = 0, texHeight = 0, channels = 0;
        std::vector<stbi_uc *> imageData(texturePaths.size());

        // Load all images first to determine dimensions
        for (size_t i = 0; i < texturePaths.size(); i++) {
            std::string path = fixPathSlashes(texturePaths[i]);
            stbi_set_flip_vertically_on_load(true);
            imageData[i] = stbi_load(path.c_str(), &texWidth, &texHeight, &channels, STBI_rgb_alpha);
            if (!imageData[i]) {
                std::cerr << "Failed to load texture for array: " << path << std::endl;
            }
        }

        // Determine internal format based on channels (use first valid image)
        GLint internalFormat = GL_RGBA8;
        GLint format = GL_RGBA;
        if (channels == 3) {
            internalFormat = GL_RGB8;
            format = GL_RGB;
        } else if (channels == 1) {
            internalFormat = GL_R8;
            format = GL_RED;
        }

        // Allocate the 2D array and upload each layer
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat,
                     texWidth, texHeight, (GLint) texturePaths.size(),
                     0, format, GL_UNSIGNED_BYTE, nullptr);

        for (size_t i = 0; i < texturePaths.size(); i++) {
            if (imageData[i]) {
                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                                0, 0, (GLint) i,
                                texWidth, texHeight, 1,
                                format, GL_UNSIGNED_BYTE, imageData[i]);
            }
        }

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, wrapMode);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, wrapMode);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, wrapMode);

        // Generate mipmaps for the array
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

        // Free loaded image data
        for (auto *data: imageData) {
            stbi_image_free(data);
        }

        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }

    // Bind this texture array to a specific texture unit
    void bind(GLuint textureUnit = 0) const {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
    }

    // Set the sampler uniform on a shader program to the given texture unit
    void setUniform(ShaderProgram &program, const std::string &uniformName, GLuint textureUnit = 0) {
        program.setAsActiveProgram();
        glUniform1i(program.getUniformLocation(uniformName), (GLint) textureUnit);
        uniformLocation = program.getUniformLocation(uniformName);
    }

    ~TreeTextureArray() {
        if (textureID != 0) {
            glDeleteTextures(1, &textureID);
        }
    }
};

#endif // TREE_TEXTURE_ARRAY_H
