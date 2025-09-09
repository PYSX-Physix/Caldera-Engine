#include "SimpleUI.h"
#include "imgui.h"
#include "../renderer/Texture.h"

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

    void Image(unsigned int texId, float w, float h) {
        ImGui::Image((void*)(uintptr_t)texId, ImVec2(w, h));
    }
    void Image(::Texture* t, float w, float h) {
        if (!t) return;
        Image(t->ID, w, h);
    }

    void PanelWithBorder(const std::string &title, float x, float y, float w, float h, ImVec4 borderColor) {
        ImGui::SetNextWindowPos(ImVec2(x,y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(w,h), ImGuiCond_Always);
        ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
        ImVec2 p = ImGui::GetWindowPos();
        ImVec2 s = ImGui::GetWindowSize();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRect(p, ImVec2(p.x + s.x, p.y + s.y), ImColor(borderColor));
        ImGui::End();
    }
}
