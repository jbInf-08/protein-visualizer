cmake_minimum_required(VERSION 3.16)

# Must precede project(): initialize all targets (including FetchContent deps) on /MD so nothing pulls LIBCMT (LNK4098).
if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

project(ProteinVisualizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG a74efa0d5628b74adc0426af4c5710e287fa7c2c
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 0af55ccecd98d4e5a8d1fad7de25ba429d60e863
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG 8199457a7d9e453f8d3d9cadc14683fb54a858b5
)
FetchContent_MakeAvailable(imgui)

add_library(glad STATIC "${CMAKE_SOURCE_DIR}/external/glad/src/glad.c")
target_include_directories(glad PUBLIC "${CMAKE_SOURCE_DIR}/external/glad/include")

set(IMGUI_DIR "${imgui_SOURCE_DIR}")
add_library(imgui STATIC
  "${IMGUI_DIR}/imgui.cpp"
  "${IMGUI_DIR}/imgui_draw.cpp"
  "${IMGUI_DIR}/imgui_tables.cpp"
  "${IMGUI_DIR}/imgui_widgets.cpp"
  "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
  "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)
target_include_directories(imgui PUBLIC "${IMGUI_DIR}" "${IMGUI_DIR}/backends")
target_include_directories(imgui PRIVATE "${CMAKE_SOURCE_DIR}/external/glad/include")
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD)
target_link_libraries(imgui PUBLIC glfw glad)

find_package(OpenGL REQUIRED)

set(SOURCES
  src/main.cpp
  src/core/Application.cpp
  src/core/Screenshot.cpp
  src/core/Window.cpp
  src/renderer/Camera.cpp
  src/renderer/Renderer.cpp
  src/model/Protein.cpp
  src/utils/PDBParser.cpp
  src/utils/MmCifParser.cpp
  src/utils/StructureLoader.cpp
  src/utils/Picking.cpp
)

add_executable(ProteinVisualizer ${SOURCES})
target_include_directories(ProteinVisualizer PRIVATE "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/external")
target_link_libraries(ProteinVisualizer PRIVATE
  OpenGL::GL
  glfw
  glad
  glm::glm
  imgui
)

if(MSVC)
  foreach(_t glad imgui glfw glm)
    if(TARGET ${_t})
      set_property(TARGET ${_t} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()
  endforeach()
  set_property(TARGET ProteinVisualizer PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
  # One dependency can still advertise LIBCMT as a default library; drop it so the link matches /MD.
  target_link_options(ProteinVisualizer PRIVATE "/NODEFAULTLIB:LIBCMT")
endif()

# Initial copy at configure time (handy for IDE generators); POST_BUILD keeps outputs in sync when editing assets.
file(COPY "${CMAKE_SOURCE_DIR}/shaders/" DESTINATION "${CMAKE_BINARY_DIR}/shaders")
file(COPY "${CMAKE_SOURCE_DIR}/resources/" DESTINATION "${CMAKE_BINARY_DIR}/resources")

add_custom_command(
  TARGET ProteinVisualizer POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/shaders"
          "$<TARGET_FILE_DIR:ProteinVisualizer>/shaders"
  COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/resources"
          "$<TARGET_FILE_DIR:ProteinVisualizer>/resources"
  COMMENT "Copy shaders and resources beside ProteinVisualizer"
)

enable_testing()
add_executable(parser_smoke_test
  tests/pdb_parser_smoke.cpp
  src/model/Protein.cpp
  src/utils/PDBParser.cpp
  src/utils/MmCifParser.cpp
  src/utils/StructureLoader.cpp
)
target_include_directories(parser_smoke_test PRIVATE "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/external")
target_link_libraries(parser_smoke_test PRIVATE glm::glm)
add_test(NAME parser_smoke_test COMMAND parser_smoke_test "${CMAKE_SOURCE_DIR}/resources/1CRN.pdb")
