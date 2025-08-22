#include "Shader.h"
#include <GL/glew.h>
#include <iostream>

Shader::Shader(const char* vertexSrc, const char* fragmentSrc) {
    ID = CreateProgram(vertexSrc, fragmentSrc);
}

Shader::~Shader() {
    if (ID) glDeleteProgram(ID);
}

unsigned int Shader::CompileShader(unsigned int type, const char* src) const {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    int success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[1024];
        glGetShaderInfoLog(id, 1024, nullptr, info);
        std::cerr << "Shader compile error: " << info << std::endl;
    }
    return id;
}

unsigned int Shader::CreateProgram(const char* vsrc, const char* fsrc) const {
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vsrc);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fsrc);
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    // Bind attribute locations explicitly for compatibility with GLSL 150
    glBindAttribLocation(program, 0, "aPos");
    glBindAttribLocation(program, 1, "aNormal");
    glBindAttribLocation(program, 2, "aTex");
    glLinkProgram(program);
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info[1024];
        glGetProgramInfoLog(program, 1024, nullptr, info);
        std::cerr << "Program link error: " << info << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

void Shader::Use() const { glUseProgram(ID); }

void Shader::SetMat4(const std::string &name, const glm::mat4 &mat) const {
    int loc = glGetUniformLocation(ID, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]);
}

void Shader::SetVec3(const std::string &name, const glm::vec3 &vec) const {
    int loc = glGetUniformLocation(ID, name.c_str());
    glUniform3fv(loc, 1, &vec[0]);
}

void Shader::SetFloat(const std::string &name, float v) const {
    int loc = glGetUniformLocation(ID, name.c_str());
    glUniform1f(loc, v);
}

void Shader::SetInt(const std::string &name, int v) const {
    int loc = glGetUniformLocation(ID, name.c_str());
    glUniform1i(loc, v);
}
