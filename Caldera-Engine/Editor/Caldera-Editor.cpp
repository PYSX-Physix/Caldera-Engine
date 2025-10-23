#include "Caldera-Editor.h"
#include "imgui.h"

void EditorBase::ConstructEditorLayout()
{
	CreateEditorViewport();
}


void EditorBase::CreateEditorViewport()
{
	{
		ImGui::Begin("Viewport", &showViewport, ImGuiWindowFlags_DockNodeHost);
		ImGui::End();
	}
}


