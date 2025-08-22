#pragma once
#include <vector>
#include <string>
#include "Mesh.h"
#include <glm/glm.hpp>

class Model {
public:
    std::vector<Mesh> meshes;
    std::vector<glm::mat4> meshTransforms;
    std::string directory;
    Model() = default;
    bool LoadModel(const std::string &path); // returns true on success
    void Draw() const;
    size_t MeshCount() const { return meshes.size(); }
    glm::vec3 GetMeshPosition(size_t i) const;
    void SetMeshPosition(size_t i, const glm::vec3 &pos);
private:
    Mesh ProcessMesh(const void* mesh, const void* scene);
};
