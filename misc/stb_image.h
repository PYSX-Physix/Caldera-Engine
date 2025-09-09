#pragma once
#include <cstddef>

// Minimal stub for stb_image to avoid bundling external copyrighted sources.
// This stub always fails to load images so the engine will use the fallback checker.
inline unsigned char* stbi_load(const char* /*filename*/, int* w, int* h, int* comp, int req_comp) {
    (void)w; (void)h; (void)comp; (void)req_comp;
    return nullptr; // signal failure to trigger fallback
}
inline void stbi_image_free(void* p) { (void)p; }
