#pragma once

#include <filesystem>

class EditorContentBrowser {
public:
    void SetRootDirectory(const std::filesystem::path& root);
    void Render(bool* isOpen);

private:
    std::filesystem::path rootPath;
    std::filesystem::path currentDirectory;


    void DrawDirectoryTree(const std::filesystem::path& path);
    void DrawAssetGrid(const std::filesystem::path& path);
};
