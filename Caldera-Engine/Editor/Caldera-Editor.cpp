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
        contentBrowser.Render(&showContentBrowser);
    }
}

EditorBase::EditorBase() : renderer(nullptr)
{
    contentBrowser.SetRootDirectory(std::filesystem::current_path());
}

void EditorBase::SetRenderer(Renderer* r)
{
    renderer = r;
    contentBrowser.SetRenderer(r);
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
        // Begin the viewport window with resizing capability
        ImGui::Begin("Viewport", &showViewport, ImGuiWindowFlags_NoScrollbar);

        // Get the size of the available content region in the window
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();

        // Ensure we have a valid renderer
        if (renderer && renderer->swapChain) {
            // Update the renderer viewport size if needed
            renderer->SetViewportSize(viewportSize.x, viewportSize.y);

            // Get the current back buffer as a texture ID for ImGui
            ImTextureID textureID = renderer->GetCurrentBackBufferImGui();

            // Display the rendered texture, maintaining aspect ratio
            ImGui::Image(textureID, viewportSize);

            // Handle viewport focusing and input capture
            bool isViewportHovered = ImGui::IsItemHovered();
            bool isViewportFocused = ImGui::IsWindowFocused();

            // You can use these states to handle input specifically for the viewport
            if (isViewportFocused && isViewportHovered) {
                // Handle viewport-specific input here if needed
            }
        }
        else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No renderer available!");
        }

        ImGui::End();
    }
}