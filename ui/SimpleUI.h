#pragma once
#include <string>
#include "../renderer/Texture.h"
#include "imgui.h"

namespace SimpleUI {
    void BeginFrame();
    void EndFrame();
    void Text(const std::string &s);
    bool Button(const std::string &s);
    void Panel(const std::string &title, float x, float y, float w, float h);
    // draw a texture by GL id
    void Image(unsigned int texId, float w, float h);
    // draw a texture from engine Texture pointer
    void Image(::Texture* t, float w, float h);
    // panel with optional border (color rgba)
    void PanelWithBorder(const std::string &title, float x, float y, float w, float h, ImVec4 borderColor);
}
