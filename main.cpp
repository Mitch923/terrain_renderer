#include <GL/glew.h>
#include <GL/gl.h>

#include "headers/shader.h"
#include "headers/texture.h"
#include "headers/mesh.h"
#include "headers/patch_mesh.h"
#include "headers/window.h"
#include "headers/basic_objects.h"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const unsigned int MAX_LIGHTS = 10; // Don't attempt to change MAX_LIGHTS unless you will also change the corresponding constant in the fragment shader
const int POLL_DELAY = 17; // In milliseconds

// Tesselation LOD Settings
const GLfloat TESS_LEVEL_HIGH = 100.0f;
const GLfloat TESS_LEVEL_LOW = 30.0f;
const GLfloat TESS_DIST_MIN = 10.0f;
const GLfloat TESS_DIST_MAX = 100.0f;

// Terrain settings
const GLfloat TERRAIN_MAX_HEIGHT = 5.0f;
const GLfloat TERRAIN_FREQ = 0.005f;
const GLfloat TERRAIN_AMP = 1.0f;

float globalTime = 0.0f; // In seconds
PatchMesh terrainMesh;
std::unordered_map<std::string, Mesh> meshes;
std::unordered_map<std::string, Texture> textures;
std::vector<glm::vec3> lightPositions;
std::vector<glm::vec3> lightColours;
ShaderProgram basicProgram, tessProgram;

// Create a mesh that consists of a grid of 100 vertices arranged into square patches
PatchMesh createTerrain(float xMin=-45.0f, float zMin=-90.0f, float xStep=10.0f, float zStep=10.0f) {
    PatchMesh m = PatchMesh(4); // 4 vertices per patch
    for (unsigned int zIndex = 0; zIndex < 10; zIndex++) {
        for (unsigned int xIndex = 0; xIndex < 10; xIndex++) {
            m.addVert(glm::vec3(xMin + xIndex*xStep, 0.0f, zMin + zIndex*zStep));
            if (xIndex > 0 && zIndex > 0) {
                m.addElements({
                    (xIndex-1) + (zIndex-1)*10,
                    (xIndex-1) +  zIndex   *10,
                     xIndex    +  zIndex   *10,
                     xIndex    + (zIndex-1)*10
                });
            }
        }
    }
    m.updateBuffers();
    return m;
}

void drawTerrain(ShaderProgram& program, PatchMesh& terrainMesh) {
	glBindVertexArray(terrainMesh.vaoID);
	glPatchParameteri(GL_PATCH_VERTICES, terrainMesh.verticesPerPatch);

	glUniform3fv(program.getUniformLocation("eyePos"), 1, glm::value_ptr(window::eyePos));
	glUniform3fv(program.getUniformLocation("specularColour"), 1, glm::value_ptr(glm::vec3(1, 1, 1)));

    // Bind the grass texture to texturing unit 0
    glActiveTexture(GL_TEXTURE0);
    textures["grass"].bind();
    glActiveTexture(GL_TEXTURE1);
    // Specular texture here
    glActiveTexture(GL_TEXTURE2);
    textures["rock"].bind();
    glActiveTexture(GL_TEXTURE3);
    textures["snow"].bind();
    glActiveTexture(GL_TEXTURE4);
    textures["sand"].bind();

    // Draw the terrain
	glDrawElements(GL_PATCHES, terrainMesh.elements.size(), GL_UNSIGNED_INT, NULL);
}

// Sets camera and lighting uniforms on a shader program. Should be run once per frame to keep camera parameters current
void setCameraAndLightUniforms(ShaderProgram& program, glm::mat4& proj, glm::mat4& view,
							   std::vector<glm::vec3>& lightPositions, std::vector<glm::vec3>& lightColours) {
	glUniformMatrix4fv(program.getUniformLocation("viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(program.getUniformLocation("projMatrix"), 1, GL_FALSE, glm::value_ptr(proj));

	unsigned int numLights = std::min(MAX_LIGHTS, (unsigned int)lightPositions.size()); //only accept up to MAX_LIGHTS lights
	glUniform1ui(program.getUniformLocation("numLights"), numLights);
	std::vector<glm::vec3> lightPositionsEyeSpace;
	for (unsigned int i = 0; i < numLights; i++) lightPositionsEyeSpace.push_back(glm::vec3(view * glm::vec4(lightPositions[i], 1.0f)));
	glUniform3fv(program.getUniformLocation("lightPositionsEyeSpace"), numLights, glm::value_ptr(lightPositionsEyeSpace[0]));
	glUniform3fv(program.getUniformLocation("lightColours"), numLights, glm::value_ptr(lightColours[0]));
}

// Draws a non-tesselated mesh using primitive types other than GL_PATCHES. Not compatible with meshes made of patches (at least without some editing)
void drawMesh(ShaderProgram& program, Mesh& mesh, glm::mat4& view) {
    // Set model transform and normal transform (transpose inverse model-view matrix)
	glm::mat4 normalMatrix = glm::inverse(view * mesh.modelTransform);
	glUniformMatrix4fv(program.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(mesh.modelTransform));
	glUniformMatrix4fv(program.getUniformLocation("normalMatrix"), 1, GL_TRUE, glm::value_ptr(normalMatrix));
	glUniform3fv(program.getUniformLocation("specularColour"), 1, glm::value_ptr(mesh.specColour));

    // Set whether a diffuse texture should be used, and if so, bind the texture to the correct texturing unit
	glUniform1i(program.getUniformLocation("applyDiffuseTexture"), mesh.diffuseTexture.isValid());
	if (mesh.diffuseTexture.isValid()) {
		glActiveTexture(GL_TEXTURE0 + 0);
		mesh.diffuseTexture.bind();
	}

    // Set whether a specular texture should be used, and if so, bind the texture to the correct texturing unit
	glUniform1i(program.getUniformLocation("applySpecularTexture"), mesh.specTexture.isValid());
	if (mesh.specTexture.isValid()) {
		glActiveTexture(GL_TEXTURE0 + 1);
		mesh.specTexture.bind();
	}

    // Bind the VAO of the mesh and then call the correct draw command (glDrawElements if element indices are specified, otherwise glDrawArrays)
	glBindVertexArray(mesh.vaoID);
	if (mesh.elements.size() > 0) glDrawElements(mesh.primitiveType, mesh.elements.size(), GL_UNSIGNED_INT, NULL);
	else 						  glDrawArrays(mesh.primitiveType, 0, mesh.verts.size());
}

// Setup GL state, load and compile shaders, load required data
void init() {
	// Initialize OpenGL state
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    window::eyePos = glm::vec3(0.0f, 10.0f, 0.0f);
    window::moveSpeed = 0.01f;

    // Load the tesselation shader program
    tessProgram = ShaderProgram({
		{"../shaders/terrain.vert", GL_VERTEX_SHADER},
		{"../shaders/terrain.cont", GL_TESS_CONTROL_SHADER},
		{"../shaders/terrain.eval", GL_TESS_EVALUATION_SHADER},
		{"../shaders/terrain.frag", GL_FRAGMENT_SHADER},
    });

    // The correct program must be set as active when uniform values corresponding to that program are modified
    tessProgram.setAsActiveProgram();
	// Textures sampler locations for the tesselation program
	glUniform3fv(tessProgram.getUniformLocation("eyePos"), 1, glm::value_ptr(window::eyePos));
	glUniform1f(tessProgram.getUniformLocation("heightMap"), 0); // heightmap texture sampler will use texturing unit 0
	glUniform1f(tessProgram.getUniformLocation("levelHigh"), TESS_LEVEL_HIGH);
	glUniform1f(tessProgram.getUniformLocation("levelLow"), TESS_LEVEL_LOW);
	glUniform1f(tessProgram.getUniformLocation("distMin"), TESS_DIST_MIN);
	glUniform1f(tessProgram.getUniformLocation("distMax"), TESS_DIST_MAX);

	// Terrain Generation Uniforms
	glUniform1f(tessProgram.getUniformLocation("terrainMaxHeight"), TERRAIN_MAX_HEIGHT);
	glUniform1f(tessProgram.getUniformLocation("terrainFrequency"), TERRAIN_FREQ);
	glUniform1f(tessProgram.getUniformLocation("terrainAmplitude"), TERRAIN_AMP);

	// Terrain Lighting Uniforms
	glUniform1i(tessProgram.getUniformLocation("diffuseTexture"), 0);
	glUniform1i(tessProgram.getUniformLocation("specularTexture"), 1);
	glUniform1i(tessProgram.getUniformLocation("rockTexture"), 2);
	glUniform1i(tessProgram.getUniformLocation("snowTexture"), 3);
	glUniform1i(tessProgram.getUniformLocation("sandTexture"), 4);
	glUniform1i(tessProgram.getUniformLocation("applyDiffuseTexture"), GL_TRUE);
	glUniform1i(tessProgram.getUniformLocation("applySpecularTexture"), GL_FALSE);

    // Create terrain
    const float terrainSeed = rand();
    terrainMesh = createTerrain();

    // Terrain Texturing
    textures["grass"] = Texture("../assets/terrain_textures/grass/leafy_grass_diff_1k.jpg", GL_REPEAT);
    textures["snow"] = Texture("../assets/terrain_textures/snow/snow_02_diff_1k.jpg", GL_REPEAT);
    textures["sand"] = Texture("../assets/terrain_textures/sand/aerial_beach_01_diff_1k.jpg", GL_REPEAT);
    textures["rock"] = Texture("../assets/terrain_textures/rock/rocks_ground_06_diff_1k.jpg", GL_REPEAT);
	// textures["heightMap"] = Texture("../assets/Terrain_hm_01.tga", GL_CLAMP_TO_EDGE);

	// Load the basic (vertex-fragment) shader program
	basicProgram = ShaderProgram({
		{"../shaders/phong_blinn.vert", GL_VERTEX_SHADER},
		{"../shaders/phong_blinn.frag", GL_FRAGMENT_SHADER}
	});

	basicProgram.setAsActiveProgram();
	// Textures sampler locations for the basic (vertex-fragment only) program
	glUniform1i(basicProgram.getUniformLocation("diffuseTexture"), 0); // diffuse texture sampler will use texturing unit 0
	glUniform1i(basicProgram.getUniformLocation("specularTexture"), 1); // specular texture sampler will use texturing unit 1

    textures["sky"] = Texture("../assets/solarsystemscope/8k_stars_milky_way.jpg", GL_CLAMP_TO_EDGE);
    meshes["skySphere"] = basic_objects::makeSphere(9000.0f, glm::vec3(1.0f), 16, 8, true);
    meshes["skySphere"].diffuseTexture = textures["sky"];
    meshes["skySphere"].specColour = glm::vec3(0.0f); // No specular reflections on the sky sphere

	textures["earth_diff"] = Texture("../assets/solarsystemscope/2k_earth_daymap.jpg", GL_CLAMP_TO_EDGE);
	textures["earth_spec"] = Texture("../assets/solarsystemscope/2k_earth_specular_map.png", GL_CLAMP_TO_EDGE);
	meshes["earth"] = basic_objects::makeSphere(10.0f, glm::vec3(1.0f));
	meshes["earth"].modelTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-100.0f, 150.0f, -400.0f));
	meshes["earth"].diffuseTexture = textures["earth_diff"];
	meshes["earth"].specTexture = textures["earth_spec"];

	lightPositions.push_back(glm::vec3(100.0f, 200.0f, 400.0f));
	lightColours.push_back(glm::vec3(0.6f, 0.5625f, 0.4875f));

	lightPositions.push_back(glm::vec3(-500.0f, 100.0f, -100.0f));
	lightColours.push_back(glm::vec3(0.5f, 0.35f, 0.15f));
}

// Frame update function - run at ~60fps (based on POLL_DELAY), update necessary uniforms and queue a new frame to render
void update(int value) {
	glutTimerFunc(POLL_DELAY, update, 0);
	glutPostRedisplay();

	if (!window::paused) globalTime += 0.001f * POLL_DELAY;

	meshes["skySphere"].modelTransform = glm::translate(glm::mat4(1.0f), window::eyePos); // Keep sky sphere centred on viewer

	window::updateUserPosition(POLL_DELAY); // Updates user position and view transform based on keyboard/mouse inputs
}

// Display function - dispatch display tasks to the GPU (vertex-fragment shader program for each VAO)
void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	basicProgram.setAsActiveProgram();
	setCameraAndLightUniforms(basicProgram, window::proj, window::view, lightPositions, lightColours);
	for (auto it = meshes.begin(); it != meshes.end(); it++) drawMesh(basicProgram, it->second, window::view);

    tessProgram.setAsActiveProgram();
	setCameraAndLightUniforms(tessProgram, window::proj, window::view, lightPositions, lightColours);
	drawTerrain(tessProgram, terrainMesh);

	glutSwapBuffers();
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitContextVersion(4, 1);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(window::width, window::height);
	glutCreateWindow("COSC422 Terrain Renderer");

	if (glewInit() != GLEW_OK) {
		std::cerr << "Unable to initialize GLEW  ...exiting." << std::endl;
		exit(EXIT_FAILURE);
	}

	window::wireframe = true;
	window::init(); // Register user input callbacks, initialize view and projection matrices
	basic_objects::init(); // Set correct GL parameters for drawing objects defined in basic_objects.h
	init(); // Load shaders, textures, meshes, and models

	glutDisplayFunc(display);
	glutTimerFunc(POLL_DELAY, update, 0);
	glutMainLoop();
}
