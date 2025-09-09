#pragma once
#include <string>

namespace SimpleUI {
    void BeginFrame();
    void EndFrame();
    void Text(const std::string &s);
    bool Button(const std::string &s);
    void Panel(const std::string &title, float x, float y, float w, float h);
}
