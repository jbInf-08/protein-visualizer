cmake_minimum_required(VERSION 3.10)
project(ProteinVisualizer)
set(CMAKE_CXX_STANDARD 17)

# Find packages
find_package(OpenGL REQUIRED)
find_package(GLM REQUIRED)

# GLFW
add_subdirectory(external/glfw)
include_directories(external/glfw/include)

# GLAD
add_library(glad external/glad/src/glad.c)
include_directories(external/glad/include)

# ImGui
add_subdirectory(external/imgui)
include_directories(external/imgui)

# Source files
file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(ProteinVisualizer ${SOURCES})

# Link libraries
target_link_libraries(ProteinVisualizer
    OpenGL::GL
    glfw
    glad
    glm::glm
    imgui
)

# Shaders and resources
file(COPY shaders/ DESTINATION ${CMAKE_BINARY_DIR}/shaders)
file(COPY resources/ DESTINATION ${CMAKE_BINARY_DIR}/resources) 