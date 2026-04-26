#pragma once

#include "../model/Protein.h"
#include "../renderer/Renderer.h"

#include <glm/mat4x4.hpp>
#include <optional>

namespace Picking {

/// Screen-space mouse position in framebuffer pixels (origin top-left, same as GLFW).
/// Returns closest hit atom index along the view ray, or `std::nullopt` if none.
std::optional<int> PickAtomIndexScreen(const Protein& protein, float mouseX, float mouseY, int framebufferWidth,
                                         int framebufferHeight, const glm::mat4& view, const glm::mat4& projection,
                                         VisualizationMode mode);

} // namespace Picking
