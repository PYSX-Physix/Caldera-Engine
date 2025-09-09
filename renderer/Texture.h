#pragma once
#include <string>

class Texture {
public:
    Texture() = default;
    ~Texture();
    bool LoadFromFile(const std::string &path);
    void Bind(unsigned int slot = 0) const;
    unsigned int ID = 0;
};
