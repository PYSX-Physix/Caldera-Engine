#include "Caldera-Editor.h"
#include "imgui.h"
#include "EditorContentBrowser.h"
#include "Renderer.h"

void EditorBase::ConstructEditorLayout()
{
    CreateWindowMenu();
    CreateEditorViewport();
    ConstructContentBrowser();
}

void EditorBase::ConstructContentBrowser()
{
    if (showContentBrowser == true)
    {
        contentbrowser.Render(&showContentBrowser);
    }
}

EditorBase::EditorBase() : renderer(nullptr)
{
    contentbrowser.SetRootDirectory(std::filesystem::current_path());
}

void EditorBase::SetRenderer(Renderer* r)
{
    renderer = r;
    contentbrowser.SetRenderer(r);
}

void EditorBase::CreateWindowMenu()
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                // Action for "Open"
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                // Action for "Save"
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                // Action for "Undo"
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z")) {
                // Action for "Redo"
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Viewport", NULL, showViewport))
            {
                showViewport = !showViewport;
            }
            if (ImGui::MenuItem("Content Browser", NULL, showContentBrowser))
            {
                showContentBrowser = !showContentBrowser;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void EditorBase::CreateEditorViewport()
{
    if (showViewport == true) {
        ImGui::Begin("Viewport", &showViewport);
        ImGui::Text("This is just a test");
        ImGui::End();
    }
}