#include "SimpleUI.h"
#include "imgui.h"

namespace SimpleUI {
    void BeginFrame() {
        // caller should call ImGui NewFrame()
    }
    void EndFrame() {
        // caller should call ImGui Render()
    }
    void Text(const std::string &s) { ImGui::TextUnformatted(s.c_str()); }
    bool Button(const std::string &s) { return ImGui::Button(s.c_str()); }
    void Panel(const std::string &title, float x, float y, float w, float h) {
        ImGui::SetNextWindowPos(ImVec2(x,y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(w,h), ImGuiCond_Always);
        ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
        ImGui::End();
    }
}
