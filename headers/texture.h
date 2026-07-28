/* COSC422 Advanced Computer Graphics (2026)

texture.h: Provides a Texture class that handles loading of images into OpenGL textures (using stb_image.h).
*/

#ifndef TEXTURE_H
#define TEXTURE_H

#include <iostream>
#include <filesystem>
#include <GL/freeglut.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Modify a file path to use the correct forward or back slash depending on the OS
std::string fixPathSlashes(std::string filePath) {
    for (char& c : filePath) if (c == '/' || c == '\\') c = std::filesystem::path::preferred_separator;
    return filePath;
}

class Texture {
    public:
        Texture() {}
        // Load from file
        Texture(std::string filePath, GLuint wrapMode, bool flip_y=true, bool verbose=true) {
            filePath = fixPathSlashes(filePath);
            int ok = stbi_info(filePath.c_str(), &width, &height, &channels);
            setFormat();
            if (ok != 1 || channels == -1) {
                std::cerr << "Failed to load " << filePath << std::endl;
                return;
            }

            // OpenGL textures expect the y dimension to be oriented opposite to how it is defined in most images, so we will flip y by default
            stbi_set_flip_vertically_on_load(flip_y);
            unsigned char* imageData = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
            if (verbose) std::cout << "Loaded texture " << filePath << " (" << channels << " channels, " << width << "x" << height << "px)" << std::endl;
            setupBuffers(imageData, wrapMode);

            stbi_image_free(imageData);
        }
        // Load from vector
        Texture(std::vector<unsigned char>& imageData, GLuint wrapMode, int width, int height, int channels) {
            this->width = width;
            this->height = height;
            this->channels = channels;
            setFormat();
            setupBuffers(&imageData[0], wrapMode);
        }

        void bind() {
            glBindTexture(GL_TEXTURE_2D, texId);
        }

        bool isValid() {
            return channels != -1;
        }
    private:
        GLuint texId, format;
        int width, height, channels = -1; // Invalid or uninitialized texture will have -1 channels

        void setFormat() {
            switch (channels) {
                case 1: format = GL_RED; break;
                case 3: format = GL_RGB; break;
                case 4: format = GL_RGBA; break;
                default: channels = -1;
            }
        }

        void setupBuffers(unsigned char* imageData, GLuint wrapMode) {
            glGenTextures(1, &texId);
            bind();
            // Send the image data into an OpenGL buffer that can be accessed inside shaders
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, imageData);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
        }
};

#endif // TEXTURE_H