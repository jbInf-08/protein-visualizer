#pragma once

#include <string>

/// Saves the current OpenGL framebuffer (RGBA readback) as an 8-bit RGB PNG.
bool SaveFramebufferPngRgb(int width, int height, const std::string& filepath);
