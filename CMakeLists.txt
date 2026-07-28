cmake_minimum_required(VERSION 3.10)

project(cosc422)

add_executable(terrain_renderer main.cpp)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(glm REQUIRED)
find_package(GLEW REQUIRED)

include_directories( ${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIRS} ${GLM_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS})

target_link_libraries( terrain_renderer ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${GLM_LIBRARY} ${GLEW_LIBRARIES})

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()
