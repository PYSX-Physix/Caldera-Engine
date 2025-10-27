#include "EditorContentBrowser.h"
#include "imgui.h"

void EditorContentBrowser::SetRootDirectory(const std::filesystem::path& root) {
    rootPath = root;
    currentDirectory = root;
}

void EditorContentBrowser::Render(bool* isOpen) {
    if (!ImGui::Begin("Content Browser", isOpen)) {
        ImGui::End();
        return;
    }

    ImGui::Columns(2, nullptr, true);

    // Left: Directory Tree
    ImGui::BeginChild("Directories", ImVec2(0, 0), true);
    DrawDirectoryTree(rootPath);
    ImGui::EndChild();

    ImGui::NextColumn();

    // Right: Asset Grid
    ImGui::BeginChild("Assets", ImVec2(0, 0), true);
    DrawAssetGrid(currentDirectory);
    ImGui::EndChild();

    ImGui::End();
}


void EditorContentBrowser::DrawDirectoryTree(const std::filesystem::path& path) {
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_directory()) {
            const std::string name = entry.path().filename().string();
            ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
            bool nodeOpen = ImGui::TreeNodeEx(name.c_str(), nodeFlags);

            if (ImGui::IsItemClicked()) {
                currentDirectory = entry.path();
            }

            if (nodeOpen) {
                DrawDirectoryTree(entry.path());
                ImGui::TreePop();
            }

        }
    }
}


void EditorContentBrowser::DrawAssetGrid(const std::filesystem::path& path) {
    int columns = 4;
    ImGui::Columns(columns, nullptr, false);

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (!entry.is_directory()) {
            const std::string name = entry.path().filename().string();
            ImGui::Button(name.c_str(), ImVec2(100, 100)); // Placeholder for thumbnail
            ImGui::NextColumn();
        }
    }

    ImGui::Columns(1);
}

