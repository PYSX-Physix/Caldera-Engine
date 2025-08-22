#pragma once
#include <string>
#include <glm/glm.hpp>

class Shader {
public:
    unsigned int ID = 0;
    Shader() = default;
    Shader(const char* vertexSrc, const char* fragmentSrc);
    ~Shader();
    void Use() const;
    void SetMat4(const std::string &name, const glm::mat4 &mat) const;
    void SetVec3(const std::string &name, const glm::vec3 &vec) const;
    void SetFloat(const std::string &name, float v) const;
    void SetInt(const std::string &name, int v) const;
private:
    unsigned int CompileShader(unsigned int type, const char* src) const;
    unsigned int CreateProgram(const char* vsrc, const char* fsrc) const;
};
