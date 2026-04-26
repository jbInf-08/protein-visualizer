#include "Screenshot.h"

#include <glad/glad.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <vector>

bool SaveFramebufferPngRgb(int width, int height, const std::string& filepath) {
    if (width <= 0 || height <= 0) {
        return false;
    }
    std::vector<unsigned char> rgba(static_cast<size_t>(width * height * 4));
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

    std::vector<unsigned char> rgb(static_cast<size_t>(width * height * 3));
    for (int y = 0; y < height; ++y) {
        const int srcY = height - 1 - y;
        for (int x = 0; x < width; ++x) {
            const size_t si = static_cast<size_t>((srcY * width + x) * 4);
            const size_t di = static_cast<size_t>((y * width + x) * 3);
            rgb[di + 0] = rgba[si + 0];
            rgb[di + 1] = rgba[si + 1];
            rgb[di + 2] = rgba[si + 2];
        }
    }

    const int stride = width * 3;
    return stbi_write_png(filepath.c_str(), width, height, 3, rgb.data(), stride) != 0;
}
