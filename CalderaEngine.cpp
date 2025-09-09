#include "imgui.h"
#include "imconfig.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "renderer/Renderer.h"
#include "renderer/Model.h"
#include "renderer/Camera.h"
#include <map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstring>
#include "engine/InputSystem.h"
#include "engine/GameInputSystem.h"
#include "engine/PlayerController.h"
#include "engine/Character.h"
#include "ui/SimpleUI.h"
#include "renderer/Texture.h"

using namespace std;

int main() {
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    // MacOS for some reason likes 3.2 but not other versions
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on macOS
    #endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Caldera Engine", NULL, NULL);
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        cerr << "Failed to initialize GLEW" << endl;
        return -1;
    }


    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

    // Setup ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150"); // macOS-friendly GLSL version

    // Setup renderer
    Renderer renderer;
    if (!renderer.Init()) cerr << "Renderer init failed" << endl;
    Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));

    // -- input / -- timing state
    float lastFrameTime = 0.0f;
    float deltaTime = 0.0f;

    InputSystem inputSys(window);
    // Game input uses the same window for now; it is a separate logical system so it can be
    // attached/detached independently in the future when creating a separate play window.
    GameInputSystem gameInput(window);
    Character playerChar;
    PlayerController controller(&inputSys, &gameInput, &camera, &playerChar);

    // Scene objects (no lights by default)
    struct SceneObject {
        enum Type { MeshObj, LightObj } type;
        string name;
        // mesh resource path
        string resource;
        // simple material: diffuse texture path
        glm::vec3 position{0.0f};
        glm::vec3 rotation{0.0f};
        glm::vec3 scale{1.0f};
        struct Material { string DiffusePath; Texture* DiffuseTex = nullptr; } material;
        // light data
        Light light;
    };

    vector<SceneObject> sceneObjects;
    size_t selectedObject = (size_t)-1;

    // resource cache for models
    map<string, Model> modelCache;
    auto GetModel = [&](const string &path)->Model* {
        if (path.empty()) return nullptr;
        auto it = modelCache.find(path);
        if (it != modelCache.end()) return &it->second;
        Model m;
        if (m.LoadModel(path)) {
            modelCache.emplace(path, move(m));
            return &modelCache.find(path)->second;
        }
        cerr << "Failed to load model resource: " << path << endl;
        return nullptr;
    };

    // scene file helpers (simple line-based format)
    auto SaveScene = [&](const string &filename){
        filesystem::create_directories("scenes");
        ofstream out("scenes/" + filename + ".scene");
            for (auto &o : sceneObjects) {
            if (o.type == SceneObject::MeshObj) {
                out << "MESH|" << o.name << "|" << o.resource << "|"
                    << o.position.x << "," << o.position.y << "," << o.position.z << "|"
                    << o.rotation.x << "," << o.rotation.y << "," << o.rotation.z << "|"
                    << o.scale.x << "," << o.scale.y << "," << o.scale.z << "|" 
                    << o.material.DiffusePath << "\n";
            } else {
                out << "LIGHT|" << o.name << "|"
                    << o.position.x << "," << o.position.y << "," << o.position.z << "|"
                    << o.light.Color.r << "," << o.light.Color.g << "," << o.light.Color.b << "|"
                    << o.light.Intensity << "\n";
            }
        }
        out.close();
    };

    auto LoadScene = [&](const string &filename){
        sceneObjects.clear();
        ifstream in("scenes/" + filename + ".scene");
        if (!in.is_open()) return false;
        string line;
        while (getline(in, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string token;
            getline(ss, token, '|');
            SceneObject o;
            if (token == "MESH") {
                // If the type is a mesh then read mesh data
                o.type = SceneObject::MeshObj;
                getline(ss, o.name, '|');
                getline(ss, o.resource, '|');
                string pos, rot, scl;
                getline(ss, pos, '|'); getline(ss, rot, '|'); getline(ss, scl, '|');
                sscanf(pos.c_str(), "%f,%f,%f", &o.position.x, &o.position.y, &o.position.z);
                sscanf(rot.c_str(), "%f,%f,%f", &o.rotation.x, &o.rotation.y, &o.rotation.z);
                sscanf(scl.c_str(), "%f,%f,%f", &o.scale.x, &o.scale.y, &o.scale.z);
                getline(ss, o.material.DiffusePath, '|');
            } else if (token == "LIGHT") {
                // If the type is a light then read light data
                o.type = SceneObject::LightObj;
                getline(ss, o.name, '|');
                string pos, col; getline(ss, pos, '|'); getline(ss, col, '|');
                sscanf(pos.c_str(), "%f,%f,%f", &o.position.x, &o.position.y, &o.position.z);
                sscanf(col.c_str(), "%f,%f,%f", &o.light.Color.r, &o.light.Color.g, &o.light.Color.b);
                string intensity; getline(ss, intensity, '|');
                o.light.Intensity = stof(intensity);
            }
            sceneObjects.push_back(o);
        }
        in.close();
        return true;
    };

    // UI helpers
    char sceneName[128] = "default";
    char newMeshPath[256] = "assets/teapot.fbx";
    char texturePath[256] = "assets/texture.png";
    // selection & transforms
    glm::vec3 modelPos(0.0f);
    glm::vec3 modelRot(0.0f);
    glm::vec3 modelScale(1.0f);
    // load material


    // framebuffer resize callback
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int width, int height){
        glViewport(0,0,width,height);
        void* ptr = glfwGetWindowUserPointer(w);
        if (ptr) {
            Renderer* r = reinterpret_cast<Renderer*>(ptr);
            r->Resize(width, height);
        }
    });
    glfwSetWindowUserPointer(window, &renderer);

    // 5. Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll and handle events
        glfwPollEvents();

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            double currentFrame = glfwGetTime();
            deltaTime = (float)(currentFrame - lastFrameTime);
            lastFrameTime = (float)currentFrame;

            // update engine input and controller
            inputSys.Update();
            controller.Update(deltaTime);
        }
        {
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
			ImGuiID maindock_id = ImGui::GetID("MainDock");
			ImGui::DockSpaceOverViewport(maindock_id, ImGui::GetMainViewport());
            ImGui::PopStyleColor(1);
        }

        // 6. Properties / Object chooser
        {
            ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_DockNodeHost);
            ImGui::Text("Scene");
            ImGui::Separator();
            // Scene save/load
            ImGui::InputText("Scene name", sceneName, sizeof(sceneName));
            if (ImGui::Button("Save Scene")) SaveScene(sceneName);
            ImGui::SameLine();
            if (ImGui::Button("Load Scene")) LoadScene(sceneName);
            ImGui::Separator();

            // Add object chooser
            ImGui::Text("Add Object");
            static int addType = 0; // 0 = Mesh, 1 = Light
            ImGui::RadioButton("Mesh", &addType, 0); ImGui::SameLine();
            ImGui::RadioButton("Light", &addType, 1);
            ImGui::InputText("Mesh path", newMeshPath, sizeof(newMeshPath));
            if (ImGui::Button("Add Object")) {
                SceneObject o;
                if (addType == 0) {
                    o.type = SceneObject::MeshObj;
                    o.name = string("Mesh_") + to_string(sceneObjects.size());
                    o.resource = string(newMeshPath);
                } else {
                    o.type = SceneObject::LightObj;
                    o.name = string("Light_") + to_string(sceneObjects.size());
                    o.light.Color = glm::vec3(1.0f);
                    o.light.Intensity = 1.0f;
                }
                sceneObjects.push_back(o);
            }
            ImGui::Separator();

            // Selected object properties
            if (selectedObject < sceneObjects.size()) {
                SceneObject &so = sceneObjects[selectedObject];
                ImGui::Text("Selected: %s", so.name.c_str());
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::DragFloat3("Position", &so.position.x, 0.1f);
                    ImGui::DragFloat3("Rotation", &so.rotation.x, 1.0f);
                    ImGui::DragFloat3("Scale", &so.scale.x, 0.01f);
                }
                if (so.type == SceneObject::LightObj) {
                    if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::ColorEdit3("Color", &so.light.Color.x);
                        ImGui::DragFloat("Intensity", &so.light.Intensity, 0.1f, 0.0f, 100.0f);
                    }
                } else if (so.type == SceneObject::MeshObj) {
                    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
                        // show/edit diffuse texture path
                        char pathBuf[512];
                        strncpy(pathBuf, so.material.DiffusePath.c_str(), sizeof(pathBuf));
                        pathBuf[sizeof(pathBuf)-1] = '\0';
                        if (ImGui::InputText("Diffuse Path", pathBuf, sizeof(pathBuf))) {
                            so.material.DiffusePath = string(pathBuf);
                        }
                        if (ImGui::Button("Load Texture")) {
                            // load texture and assign to model/material
                            if (!so.material.DiffusePath.empty()) {
                                Texture* t = new Texture();
                                if (t->LoadFromFile(so.material.DiffusePath)) {
                                    so.material.DiffuseTex = t;
                                } else {
                                    delete t;
                                    so.material.DiffuseTex = nullptr;
                                }
                                // if model is loaded, set its texture too
                                Model* m = GetModel(so.resource);
                                if (m) m->SetTexture(so.material.DiffuseTex);
                            }
                        }
                        ImGui::Separator();
                        if (so.material.DiffuseTex) {
                            ImGui::Text("Preview:");
                            SimpleUI::Image(so.material.DiffuseTex, 128.0f, 128.0f);
                        } else {
                            ImGui::Text("No diffuse texture assigned.");
                        }
                    }
                }
            } else {
                ImGui::Text("No object selected.");
            }

            ImGui::End();
        }

        {
            ImGui::Begin("World Outliner", nullptr, ImGuiWindowFlags_DockNodeHost);
            if (ImGui::Button("Clear")) { sceneObjects.clear(); }
            ImGui::SameLine();
            if (ImGui::Button("Reload Models")) { modelCache.clear(); }
            ImGui::Separator();
            for (size_t i = 0; i < sceneObjects.size(); ++i) {
                ImGui::PushID((int)i);
                bool isSelected = (selectedObject == i);
                if (ImGui::Selectable(sceneObjects[i].name.c_str(), isSelected)) selectedObject = i;
                ImGui::PopID();
            }
            ImGui::End();
        }
        // Rendering
        ImGui::Render();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.00f, 0.00f, 0.00f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // animate light slowly (if you want)
    float t = (float)glfwGetTime();
    //lights[0].Position = glm::vec3(cos(t) * 3.0f, 2.0f, sin(t) * 3.0f);

    // build lights list from sceneObjects
    vector<Light> lightsList;
    for (auto &o : sceneObjects) if (o.type == SceneObject::LightObj) {
        Light L = o.light;
        L.Position = o.position;
        lightsList.push_back(L);
    }

    // render each mesh object
    for (auto &o : sceneObjects) {
    if (o.type == SceneObject::MeshObj) {
            Model* mdl = GetModel(o.resource);
            if (!mdl) continue;
            glm::mat4 m(1.0f);
            m = glm::translate(m, o.position);
            m = glm::rotate(m, glm::radians(o.rotation.x), glm::vec3(1,0,0));
            m = glm::rotate(m, glm::radians(o.rotation.y), glm::vec3(0,1,0));
            m = glm::rotate(m, glm::radians(o.rotation.z), glm::vec3(0,0,1));
            m = glm::scale(m, o.scale);
            renderer.Render(camera, *mdl, lightsList, m);
        }
    }

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
        // Swap buffers
        glfwSwapBuffers(window);
    }

    // 7. Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}