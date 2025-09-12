#pragma once
#include <vector>
#include <string>
#include "Mesh.h"
#include <glm/glm.hpp>
#include "Texture.h"

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

    // Texture setters
    void SetTexture(Texture* t)       { tex = t; }
    void SetNormalMap(Texture* t)     { normalTex = t; }
    void SetRoughnessMap(Texture* t)  { roughnessTex = t; }
    void SetMetallicMap(Texture* t)   { metallicTex = t; }

    // Texture getters
    Texture* GetTexture() const       { return tex; }
    Texture* GetNormalMap() const     { return normalTex; }
    Texture* GetRoughnessMap() const  { return roughnessTex; }
    Texture* GetMetallicMap() const   { return metallicTex; }

private:
    Mesh ProcessMesh(const void* mesh, const void* scene);

    Texture* tex = nullptr;
    Texture* normalTex = nullptr;
    Texture* roughnessTex = nullptr;
    Texture* metallicTex = nullptr;
};
