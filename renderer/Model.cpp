#include "Model.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

bool Model::LoadModel(const std::string &path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        return false;
    }
    directory = path.substr(0, path.find_last_of('/'));
    std::function<void(aiNode*, const aiScene*)> recurse = [&](aiNode* node, const aiScene* sc) {
        for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
            aiMesh* mesh = sc->mMeshes[node->mMeshes[i]];
            meshes.push_back(ProcessMesh(mesh, sc));
        }
        for (unsigned int i = 0; i < node->mNumChildren; ++i) recurse(node->mChildren[i], sc);
    };
    recurse(scene->mRootNode, scene);
    // initialize mesh transforms to identity
    meshTransforms.resize(meshes.size(), glm::mat4(1.0f));
    return true;
}

Mesh Model::ProcessMesh(const void* meshPtr, const void* scenePtr) {
    const aiMesh* mesh = reinterpret_cast<const aiMesh*>(meshPtr);
    const aiScene* scene = reinterpret_cast<const aiScene*>(scenePtr);
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        Vertex v;
        v.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        if (mesh->HasNormals()) v.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        else v.Normal = glm::vec3(0.0f, 0.0f, 0.0f);
        if (mesh->mTextureCoords[0]) v.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        else v.TexCoords = glm::vec2(0.0f, 0.0f);
        vertices.push_back(v);
    }
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) indices.push_back(face.mIndices[j]);
    }
    return Mesh(vertices, indices);
}

void Model::Draw() const {
    for (const auto &m : meshes) m.Draw();
}

glm::vec3 Model::GetMeshPosition(size_t i) const {
    if (i >= meshTransforms.size()) return glm::vec3(0.0f);
    glm::mat4 t = meshTransforms[i];
    return glm::vec3(t[3][0], t[3][1], t[3][2]);
}

void Model::SetMeshPosition(size_t i, const glm::vec3 &pos) {
    if (i >= meshTransforms.size()) return;
    meshTransforms[i] = glm::translate(glm::mat4(1.0f), pos);
}
