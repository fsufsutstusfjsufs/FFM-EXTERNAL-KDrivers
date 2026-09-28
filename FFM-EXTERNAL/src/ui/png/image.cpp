#include "image.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>
#include <queue>
#include <utility>

namespace ui::image {
    bool LoadTextureFromMemory(const uint8_t* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height) {
        int w, h, ch;
        unsigned char* pixels = stbi_load_from_memory(data, data_size, &w, &h, &ch, 4);
        if (!pixels) return false;

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        stbi_image_free(pixels);

        *out_texture = texture;
        *out_width = w;
        *out_height = h;
        return true;
    }

    bool LoadTextureFromMemoryTransparent(const uint8_t* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height) {
        int w, h, ch;
        unsigned char* pixels = stbi_load_from_memory(data, data_size, &w, &h, &ch, 4);
        if (!pixels) return false;

        // Flood fill from outer perimeter to turn black background pixels into transparent alpha = 0
        std::vector<bool> visited(w * h, false);
        std::queue<std::pair<int, int>> q;

        auto is_bg = [&](int x, int y) {
            int idx = (y * w + x) * 4;
            return pixels[idx] <= 20 && pixels[idx + 1] <= 20 && pixels[idx + 2] <= 20;
        };

        for (int x = 0; x < w; x++) {
            if (is_bg(x, 0)) { q.push({x, 0}); visited[0 * w + x] = true; }
            if (is_bg(x, h - 1)) { q.push({x, h - 1}); visited[(h - 1) * w + x] = true; }
        }
        for (int y = 0; y < h; y++) {
            if (is_bg(0, y) && !visited[y * w + 0]) { q.push({0, y}); visited[y * w + 0] = true; }
            if (is_bg(w - 1, y) && !visited[y * w + (w - 1)]) { q.push({w - 1, y}); visited[y * w + (w - 1)] = true; }
        }

        const int dx[] = {1, -1, 0, 0};
        const int dy[] = {0, 0, 1, -1};

        while (!q.empty()) {
            auto pt = q.front();
            q.pop();
            int cx = pt.first;
            int cy = pt.second;

            int idx = (cy * w + cx) * 4;
            pixels[idx + 3] = 0;

            for (int dir = 0; dir < 4; dir++) {
                int nx = cx + dx[dir];
                int ny = cy + dy[dir];
                if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                    int n_idx = ny * w + nx;
                    if (!visited[n_idx] && is_bg(nx, ny)) {
                        visited[n_idx] = true;
                        q.push({nx, ny});
                    }
                }
            }
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        stbi_image_free(pixels);

        *out_texture = texture;
        *out_width = w;
        *out_height = h;
        return true;
    }
}
