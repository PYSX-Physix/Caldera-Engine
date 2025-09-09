#include "Texture.h"
#include <GL/glew.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "misc/stb_image.h"

Texture::~Texture() { if (ID) glDeleteTextures(1, &ID); }

bool Texture::LoadFromFile(const std::string &path) {
    int w,h,comp;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &comp, 4);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << ", falling back to checker." << std::endl;
        // create checker
        const int size = 8;
        unsigned char buf[size*size*4];
        for (int y=0;y<size;++y) for (int x=0;x<size;++x) {
            bool black = ((x/2+y/2)&1)==0;
            int i = (y*size + x) * 4;
            buf[i+0] = black ? 50 : 200;
            buf[i+1] = black ? 50 : 200;
            buf[i+2] = black ? 50 : 200;
            buf[i+3] = 255;
        }
        if (!ID) glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_2D, ID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }
    if (!ID) glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return true;
}

void Texture::Bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, ID);
}
