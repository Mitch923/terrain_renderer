/* COSC422 Advanced Computer Graphics (2026)

patch_mesh.h: Provides a PatchMesh class that encapsulates the data and sets up the VAO/VBO/EBO needed to draw a tesselated mesh made out of patches.
There is only one vertex attribute, which is the position of each vertex. Each PatchMesh instance stores the number of vertices that make up
each patch (verticesPerPatch), which can be used to set GL_PATCH_VERTICES to the correct value before drawing.
*/

#ifndef PATCH_H
#define PATCH_H

#include <vector>
#include <GL/freeglut.h>
#include <glm/glm.hpp>

class PatchMesh {
    private:
        GLuint vboID, eboID;
    public:
        GLuint vaoID, hint = GL_STATIC_DRAW;
        std::vector<glm::vec3> verts;
        std::vector<unsigned int> elements;
        int verticesPerPatch;

        PatchMesh() {}
        PatchMesh(int verticesPerPatch) {
            this->verticesPerPatch = verticesPerPatch;
            glGenVertexArrays(1, &vaoID);
            glGenBuffers(1, &vboID); // Buffer for vertex data
            glGenBuffers(1, &eboID); // Buffer for element data (face indices)

            // Bind the VAO for this mesh
            glBindVertexArray(vaoID);

            // Positions
            glBindBuffer(GL_ARRAY_BUFFER, vboID); 
            glEnableVertexAttribArray(0); //enable location 0 to be accessed as a per-vertex attribute within the vertex shader
            // Set vertex location 0 to point to VBO 0 (currently bound), 3 floats per vertex, step size 12 bytes (sizeof Vertex), offset 0 bytes
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0); 
        }

        // Update the GPU-accessible buffers for this mesh (the VBO and EBO)
        void updateBuffers() {
            glBindVertexArray(vaoID);
            glBindBuffer(GL_ARRAY_BUFFER, vboID);
            glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(glm::vec3), verts.data(), hint);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size()*sizeof(unsigned int), elements.data(), hint);
        }

        void addVert(glm::vec3 position) {
            verts.push_back(position);
        }

        void addElements(std::vector<unsigned int> newElements) {
            elements.insert(elements.end(), newElements.begin(), newElements.end());
        }
};

#endif // PATCH_H