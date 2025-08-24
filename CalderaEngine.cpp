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

int main() {
    // 1. Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2); // macOS prefers 3.2
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on macOS
    #endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Caldera Engine", NULL, NULL);
    glfwMakeContextCurrent(window);

    // 2. Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }


    // 2. Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

    // 3. Setup ImGui style
    ImGui::StyleColorsDark();

    // 4. Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150"); // macOS-friendly GLSL version

    // Setup renderer
    Renderer renderer;
    if (!renderer.Init()) std::cerr << "Renderer init failed" << std::endl;
    Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));

    // Scene objects (no lights by default)
    struct SceneObject {
        enum Type { MeshObj, LightObj } type;
        std::string name;
        // mesh resource path
        std::string resource;
        glm::vec3 position{0.0f};
        glm::vec3 rotation{0.0f};
        glm::vec3 scale{1.0f};
        // light data
        Light light;
    };

    std::vector<SceneObject> sceneObjects;
    size_t selectedObject = (size_t)-1;

    // resource cache for models
    std::map<std::string, Model> modelCache;
    auto GetModel = [&](const std::string &path)->Model* {
        if (path.empty()) return nullptr;
        auto it = modelCache.find(path);
        if (it != modelCache.end()) return &it->second;
        Model m;
        if (m.LoadModel(path)) {
            modelCache.emplace(path, std::move(m));
            return &modelCache.find(path)->second;
        }
        std::cerr << "Failed to load model resource: " << path << std::endl;
        return nullptr;
    };

    // scene file helpers (simple line-based format)
    auto SaveScene = [&](const std::string &filename){
        std::filesystem::create_directories("scenes");
        std::ofstream out("scenes/" + filename + ".scene");
            for (auto &o : sceneObjects) {
            if (o.type == SceneObject::MeshObj) {
                out << "MESH|" << o.name << "|" << o.resource << "|"
                    << o.position.x << "," << o.position.y << "," << o.position.z << "|"
                    << o.rotation.x << "," << o.rotation.y << "," << o.rotation.z << "|"
                    << o.scale.x << "," << o.scale.y << "," << o.scale.z << "\n";
            } else {
                out << "LIGHT|" << o.name << "|"
                    << o.position.x << "," << o.position.y << "," << o.position.z << "|"
                    << o.light.Color.r << "," << o.light.Color.g << "," << o.light.Color.b << "|"
                    << o.light.Intensity << "\n";
            }
        }
        out.close();
    };

    auto LoadScene = [&](const std::string &filename){
        sceneObjects.clear();
        std::ifstream in("scenes/" + filename + ".scene");
        if (!in.is_open()) return false;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string token;
            std::getline(ss, token, '|');
            SceneObject o;
            if (token == "MESH") {
                o.type = SceneObject::MeshObj;
                std::getline(ss, o.name, '|');
                std::getline(ss, o.resource, '|');
                std::string pos, rot, scl;
                std::getline(ss, pos, '|'); std::getline(ss, rot, '|'); std::getline(ss, scl, '|');
                sscanf(pos.c_str(), "%f,%f,%f", &o.position.x, &o.position.y, &o.position.z);
                sscanf(rot.c_str(), "%f,%f,%f", &o.rotation.x, &o.rotation.y, &o.rotation.z);
                sscanf(scl.c_str(), "%f,%f,%f", &o.scale.x, &o.scale.y, &o.scale.z);
            } else if (token == "LIGHT") {
                o.type = SceneObject::LightObj;
                std::getline(ss, o.name, '|');
                std::string pos, col; std::getline(ss, pos, '|'); std::getline(ss, col, '|');
                sscanf(pos.c_str(), "%f,%f,%f", &o.position.x, &o.position.y, &o.position.z);
                sscanf(col.c_str(), "%f,%f,%f", &o.light.Color.r, &o.light.Color.g, &o.light.Color.b);
                std::string intensity; std::getline(ss, intensity, '|');
                o.light.Intensity = std::stof(intensity);
            }
            sceneObjects.push_back(o);
        }
        in.close();
        return true;
    };

    // UI helpers
    char sceneName[128] = "default";
    char newMeshPath[256] = "assets/teapot.fbx";
    // selection & transforms
    glm::vec3 modelPos(0.0f);
    glm::vec3 modelRot(0.0f);
    glm::vec3 modelScale(1.0f);

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
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
			ImGuiID maindock_id = ImGui::GetID("MainDock");
			ImGui::DockSpaceOverViewport(maindock_id, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoResize);
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
                    o.name = std::string("Mesh_") + std::to_string(sceneObjects.size());
                    o.resource = std::string(newMeshPath);
                } else {
                    o.type = SceneObject::LightObj;
                    o.name = std::string("Light_") + std::to_string(sceneObjects.size());
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

        {
            // viewport camera controls: when mouse dragged in viewport, rotate camera
            ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
            ImVec2 vpSize = ImGui::GetContentRegionAvail();
            if (vpSize.y == 0){ vpSize.y = 1; } // avoid issues
            ImGui::InvisibleButton("canvas", vpSize);
            bool isDragging = ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left);
            if (isDragging) {
                ImVec2 md = io.MouseDelta;
                camera.ProcessMouseMovement(md.x, md.y);
            }
            ImGui::PopStyleColor(1);
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
    std::vector<Light> lightsList;
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