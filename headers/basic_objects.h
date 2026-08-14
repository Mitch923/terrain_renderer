/* COSC422 Advanced Computer Graphics (2026)

basic_object.h: Provides functions to construct some simple shapes using the Mesh class. Shapes which are composed using the GL_TRIANGLE_STRIP primitive
type require basic_objects::init() to be run before drawing the objects, since they rely on primitive restart to draw multiple triangle strips in one draw
call.
*/

#ifndef BASIC_OBJECTS_H
#define BASIC_OBJECTS_H

#include "mesh.h"
#include <glm/glm.hpp>

namespace basic_objects {
    const unsigned int PRIMITIVE_RESTART_INDEX = 0xFFFFFFFF;

    void init() {
        glEnable(GL_PRIMITIVE_RESTART);
        glPrimitiveRestartIndex(PRIMITIVE_RESTART_INDEX); // 0xFFFFFFFF
    }

    Mesh makeCuboid(float size, glm::vec3 colour) {
        Mesh cuboid = Mesh(GL_TRIANGLES);
        glm::vec3 normal;
        size *= 0.5f;

        //top side vertices
        normal = glm::vec3(0.0f, 1.0f, 0.0f);
        cuboid.addVert(glm::vec3(size, size, size),   normal, colour, glm::vec2(1.0f, 0.0f));
        cuboid.addVert(glm::vec3(size, size, -size),  normal, colour, glm::vec2(1.0f, 1.0f));
        cuboid.addVert(glm::vec3(-size, size, -size), normal, colour, glm::vec2(0.0f, 1.0f));
        cuboid.addVert(glm::vec3(-size, size, size),  normal, colour, glm::vec2(0.0f, 0.0f));

        //bottom side vertices
        normal = glm::vec3(0.0f, -1.0f, 0.0f);
        cuboid.addVert(glm::vec3(-size, -size, size),  normal, colour, glm::vec2(1.0f, 0.0f));
        cuboid.addVert(glm::vec3(-size, -size, -size), normal, colour, glm::vec2(1.0f, 1.0f));
        cuboid.addVert(glm::vec3(size, -size, -size),  normal, colour, glm::vec2(0.0f, 1.0f));
        cuboid.addVert(glm::vec3(size, -size, size),   normal, colour, glm::vec2(0.0f, 0.0f));

        //front side vertices
        normal = glm::vec3(0.0f, 0.0f, 1.0f);
        cuboid.addVert(glm::vec3(size, -size, size),  normal, colour, glm::vec2(1.0f, 0.0f));
        cuboid.addVert(glm::vec3(size, size, size),   normal, colour, glm::vec2(1.0f, 1.0f));
        cuboid.addVert(glm::vec3(-size, size, size),  normal, colour, glm::vec2(0.0f, 1.0f));
        cuboid.addVert(glm::vec3(-size, -size, size), normal, colour, glm::vec2(0.0f, 0.0f));

        //back side vertices
        normal = glm::vec3(0.0f, 0.0f, -1.0f);
        cuboid.addVert(glm::vec3(-size, -size, -size), normal, colour, glm::vec2(1.0f, 0.0f));
        cuboid.addVert(glm::vec3(-size, size, -size),  normal, colour, glm::vec2(1.0f, 1.0f));
        cuboid.addVert(glm::vec3(size, size, -size),   normal, colour, glm::vec2(0.0f, 1.0f));
        cuboid.addVert(glm::vec3(size, -size, -size),  normal, colour, glm::vec2(0.0f, 0.0f));

        //right side vertices
        normal = glm::vec3(1.0f, 0.0f, 0.0f);
        cuboid.addVert(glm::vec3(size, -size, -size),  normal, colour, glm::vec2(1.0f, 0.0f));
        cuboid.addVert(glm::vec3(size, size, -size),   normal, colour, glm::vec2(1.0f, 1.0f));
        cuboid.addVert(glm::vec3(size, size, size),    normal, colour, glm::vec2(0.0f, 1.0f));
        cuboid.addVert(glm::vec3(size, -size, size),   normal, colour, glm::vec2(0.0f, 0.0f));

        //left side vertices
        normal = glm::vec3(-1.0f, 0.0f, 0.0f);
        cuboid.addVert(glm::vec3(-size, -size, size),  normal, colour, glm::vec2(1.0f, 0.0f));
        cuboid.addVert(glm::vec3(-size, size, size),   normal, colour, glm::vec2(1.0f, 1.0f));
        cuboid.addVert(glm::vec3(-size, size, -size),  normal, colour, glm::vec2(0.0f, 1.0f));
        cuboid.addVert(glm::vec3(-size, -size, -size), normal, colour, glm::vec2(0.0f, 0.0f));

        //for each side (0, 1, 2, 3), define two triangles with indices (0, 1, 2), and (2, 3, 0)
        for (unsigned int sideIndex = 0; sideIndex < 6; sideIndex++) {
            cuboid.addElements({sideIndex*4 + 0, sideIndex*4 + 1, sideIndex*4 + 2});
            cuboid.addElements({sideIndex*4 + 2, sideIndex*4 + 3, sideIndex*4 + 0});
        }

        cuboid.updateBuffers();

        return cuboid;
    }

    Mesh makeSphere(float radius, glm::vec3 colour, int numSlices=32, int numStacks=16, bool invertNormals=false) {
        Mesh sphere = Mesh(GL_TRIANGLE_STRIP);
        float normalModifier = invertNormals ? -1.0f : 1.0f;
        unsigned int elementIndex = 0;

        // Iterate through each stack (horizontal), creating a triangle-strip for each
        for (int stack = 0; stack < numStacks; stack++) {
            // Stack angles range from -90 degrees to +90 degrees
            float theta1 = glm::radians(180.0f* stack     /numStacks - 90.0f);
            float theta2 = glm::radians(180.0f*(stack + 1)/numStacks - 90.0f);

            // Iterate through each vertical pair (slice) of points on the stack
            for (int slice = 0; slice <= numSlices; slice++) {
                // Slice angles range from 0 to 360 degrees
                float phi = glm::radians(360.0f*slice/numSlices);

                // Spherical coordinates to cartesian coordinates
                glm::vec3 lowerPosition(radius*cos(theta1)*cos(phi), radius*sin(theta1), radius*cos(theta1)*sin(phi));
                glm::vec3 upperPosition(radius*cos(theta2)*cos(phi), radius*sin(theta2), radius*cos(theta2)*sin(phi));

                sphere.addVert(lowerPosition, glm::normalize(lowerPosition)*normalModifier, colour, glm::vec2((float)slice/numSlices, (float)stack/numStacks));
                sphere.addVert(upperPosition, glm::normalize(upperPosition)*normalModifier, colour, glm::vec2((float)slice/numSlices, (float)(stack + 1)/numStacks));

                // Add elements based on normal direction (inwards or outwards)
                if (invertNormals) sphere.addElements({elementIndex+1, elementIndex});
                else               sphere.addElements({elementIndex, elementIndex+1});
                elementIndex += 2;
            }
            sphere.elements.push_back(PRIMITIVE_RESTART_INDEX);
        }

        sphere.updateBuffers();

        return sphere;
    }

    // Creates a cross-billboard tree mesh: two perpendicular quads that always face the viewer from any angle.
    // Quad 1 faces along +X axis (visible when viewing from ±Z directions)
    // Quad 2 faces along +Z axis (visible when viewing from ±X directions)
    Mesh makeCrossBillboardTree(float quadSize, glm::vec3 colour) {
        Mesh tree = Mesh(GL_TRIANGLES);

        float halfHeight = quadSize * 1.5f; // Tree is taller than wide
        float halfWidth = quadSize;

        // Quad 1: facing along +Z axis (normal in +Z direction)
        // Vertices ordered for proper winding (counter-clockwise when viewed from front)
        tree.addVert(glm::vec3(-halfWidth, -halfHeight, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), colour, glm::vec2(0.0f, 0.0f)); // bottom-left
        tree.addVert(glm::vec3(halfWidth, -halfHeight, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), colour, glm::vec2(1.0f, 0.0f)); // bottom-right
        tree.addVert(glm::vec3(halfWidth, halfHeight, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), colour, glm::vec2(1.0f, 1.0f)); // top-right
        tree.addVert(glm::vec3(-halfWidth, halfHeight, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), colour, glm::vec2(0.0f, 1.0f)); // top-left

        // Quad 2: facing along +X axis (normal in +X direction)
        tree.addVert(glm::vec3(0.0f, -halfHeight, -halfWidth), glm::vec3(1.0f, 0.0f, 0.0f), colour, glm::vec2(0.0f, 0.0f)); // bottom-left
        tree.addVert(glm::vec3(0.0f, -halfHeight, halfWidth), glm::vec3(1.0f, 0.0f, 0.0f), colour, glm::vec2(1.0f, 0.0f)); // bottom-right
        tree.addVert(glm::vec3(0.0f, halfHeight, halfWidth), glm::vec3(1.0f, 0.0f, 0.0f), colour, glm::vec2(1.0f, 1.0f)); // top-right
        tree.addVert(glm::vec3(0.0f, halfHeight, -halfWidth), glm::vec3(1.0f, 0.0f, 0.0f), colour, glm::vec2(0.0f, 1.0f)); // top-left

        // Add triangle indices for Quad 1 (two triangles)
        tree.addElements({0, 1, 2});
        tree.addElements({0, 2, 3});

        // Add triangle indices for Quad 2 (two triangles)
        tree.addElements({4, 5, 6});
        tree.addElements({4, 6, 7});

        tree.updateBuffers();

        return tree;
    }
}

#endif // BASIC_OBJECTS_H