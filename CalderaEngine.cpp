// TODO: Possibly add renaming assets in the world outliner or properties section
// TODO: Add basic terrain generation from heightmap
// TODO: Add deleting objects from scene
// TODO: Add Undo and Redo
// TODO: Add UI Designer or use external tool that the engine can read from
// TODO: Add audio system
// TODO: Add physics system
// TODO: Add option for adding sounds to scene
// TODO: Add dynamic particle system
// TODO: Add animation support for models
// TODO: Add basic skybox support
// TODO: Add more complex material support (normal maps, specular maps, etc.)
// TODO: Use a seperate window for play-in-editor mode (Allows for debuging and better separation of editor and game input)
// TODO: Add gizmos for moving, rotating, and scaling a selected object in the scene view
// TODO: Possibly create project system with the .vproject file extention
// TODO: Create custom function for C++ 
// TODO: Create content browser for importing assets, viewing them, and dragging them into the scene, and deleting them
// TODO: Add system for game starting level and editor starting level with both being different startup points
// TODO: Allow making children classes of a specific asset
// TODO: Improve the world outliner to allow for better organization of scene objects via grouping or parenting
// TODO: Improve the world outliner to allow for right clicking to edit, duplicate, or delete objects
// TODO: Implement a search function for the world outliner
// TODO: Add better error handling and reporting
// TODO: Create a build system for building the game into an executable
// TODO: Create plugin system for adding functionality to the editor, engine, or game
// TODO: Add a way to load and save scenes
// TODO: Add a loading screen when starting the game or loading a scene (This can be a plugin)
// TODO: Add a way to change the keybindings
// TODO: Create game settings system that is seperate from the engine settings (Allow for custom settings to written in a child class)
// TODO: Add a way to change the editor settings (Window size, theme, keybindings, etc.)
// TODO: Add a way to dynamically set UI scale based on DPI or user preference
// TODO: Add localization for games in different countries
// TODO: Add a way to profile the game and editor for performance issues
// TODO: Add a way to view logs and errors in the editor
// TODO: Create a way to set ui elements like text, images, buttons, etc. in C++ or the editor
// TODO: Add a way to preview UI elements in the editor
// TODO: Set a way to save and load UI layouts
// TODO: Create a way to animate UI elements
// TODO: Create system to set UI textures, fonts, colors, etc.
// TODO: Add button events for UI elements
// TODO: Create a way to set UI element positions and sizes in the editor or in external tools
// TODO: Add UI console support in future for different input types
// TODO: Create trigger systems for when something is suppoesed to happen in the game (This can be a plugin)
// TODO: Add crash reporter for when the engine or game crashes
// TODO: Add uninstall helper for game launchers
// TODO: Add sun and moon lighting system for day/night cycles (This can be a plugin, the sun and moon assets cannot be apart of the plugin)
// TODO: Create a point and click system for moving the character around the scene (This can be a plugin)
// TODO: Make the model and texture system use reletative paths instead of whole paths
// TODO: Add animation support
// TODO: Add LOD support for models
// TODO: Add a way to view and edit materials in the editor
// TODO: Add a way to view and edit shaders in the editor (Maybe post processing system?)
// TODO: Add a way to view textures in the editor and to filter the RGBA channels
// TODO: Add a way to view models in the editor and to filter the vertices, uvs, normals, etc.
// TODO: Add a way to view animations in the editor and to filter the bones, keyframes, etc.
// TODO: Add a way to view sounds in the editor like their sign waves and also adjust volume, pitch, etc.
// TODO: Create sound classes to filter what can be heard and not heard in game settings
// TODO: Allow meshes to use physics if needed (this can be changed in the properties section of the instanced mesh)
// TODO: Create patching solution for in case of hotfixes or small updates
// TODO: Use STB image for texture loading and saving
// TODO: Use SimpleUI for in-game UI and interface rendering
// TODO: Add a way to change the field of view of the camera
// TODO: Add a way to change the speed of the camera
// TODO: Create movement system for player and allow the player speed to be adjusted in code or in the editor
// TODO: Create a way to set the player character's position and rotation
// TODO: Create game start item for the player to start in specific levels/scenes
// TODO: Create navigation volume for AI systems
// TODO: Craate AI systems and allow creation of AI through C++
// TODO: Create a way to view and edit AI in the editor
// TODO: Make sure all packaged assets are stored in either .PAK files or unviewable files so game leaking isn't possible
// TODO: Create anti-tamper solution for anti-cheats
// TODO: Create anti-piracy solution for games
// TODO: Add networking/multiplayer support
// TODO: Add a way to view and edit networking/multiplayer in the editor
// TODO: Add a way to change the near and far plane of the camera
// TODO: Use C++ for coding systems (Some C++ we will built for engine and some items will be raw C++ code)
// TODO: Add water systems (This will be a plugin)
// TODO: Add volumetric fog support (This will be a plugin)
// TODO: Create C++ syntax to avoid illegal C++ code
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
        std::cerr << "Failed to initialize GLEW" << std::endl;
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
    if (!renderer.Init()) std::cerr << "Renderer init failed" << std::endl;
    Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));

    // -- input / -- timing state
    float lastFrameTime = 0.0f;
    float deltaTime = 0.0f;
    bool firstMouse = true;
    double lastMouseX = 0.0, lastMouseY = 0.0;
    bool mouseCaptured = false;
    const float movementSpeed = 3.0f;


    GameInputSystem gameInput(window);
    Character playerChar;

    // Scene objects (no lights by default)
    struct SceneObject {
        enum Type { MeshObj, LightObj, LandscapeObj } type;
        std::string name;
        // mesh resource path
        std::string resource;
        // simple material: diffuse texture path
        glm::vec3 position{0.0f};
        glm::vec3 rotation{0.0f};
        glm::vec3 scale{1.0f};
        struct Material { std::string DiffusePath; Texture* DiffuseTex = nullptr; } material;
        // light data
        Light light;
        // landscape data
        struct Terrain { Texture* Heightmap = nullptr; Texture* Material = nullptr; } terrain;
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
        return nullptr;
    };

    // scene file helpers
    auto SaveScene = [&](const std::string &filename){
        std::filesystem::create_directories("scenes");
        std::ofstream out("scenes/" + filename + ".scene");
            for (auto &o : sceneObjects) {
            if (o.type == SceneObject::MeshObj) {
                out << "MESH|" << o.name << "|" << o.resource << "|"
                    << o.position.x << "," << o.position.y << "," << o.position.z << "|"
                    << o.rotation.x << "," << o.rotation.y << "," << o.rotation.z << "|"
                    << o.scale.x << "," << o.scale.y << "," << o.scale.z << "|" 
                    << o.material.DiffusePath << "\n";
            } else if (o.type == SceneObject::LightObj) {
                out << "LIGHT|" << o.name << "|"
                    << o.position.x << "," << o.position.y << "," << o.position.z << "|"
                    << o.light.Color.r << "," << o.light.Color.g << "," << o.light.Color.b << "|"
                    << o.light.Intensity << "\n";
            } else if (o.type == SceneObject::LandscapeObj) {
                out << "LANDSCAPE|" << o.name << "|"
                    << o.position.x << "," << o.position.y << "," << o.position.z << "|"
                    << o.terrain.Heightmap << "|" << o.terrain.Material << "\n";
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
                // If the type is a mesh then read mesh data
                o.type = SceneObject::MeshObj;
                std::getline(ss, o.name, '|');
                std::getline(ss, o.resource, '|');
                std::string pos, rot, scl;
                std::getline(ss, pos, '|'); std::getline(ss, rot, '|'); std::getline(ss, scl, '|');
                sscanf(pos.c_str(), "%f,%f,%f", &o.position.x, &o.position.y, &o.position.z);
                sscanf(rot.c_str(), "%f,%f,%f", &o.rotation.x, &o.rotation.y, &o.rotation.z);
                sscanf(scl.c_str(), "%f,%f,%f", &o.scale.x, &o.scale.y, &o.scale.z);
                std::getline(ss, o.material.DiffusePath, '|');
                if (!o.material.DiffusePath.empty()) {
                    Texture* tex = new Texture();
                    if (tex->LoadFromFile(o.material.DiffusePath)) {
                        o.material.DiffuseTex = tex;
                        Model* m = GetModel(o.resource);
                        if (m) m->SetTexture(o.material.DiffuseTex);
                    } else {
                        delete tex;
                        o.material.DiffuseTex = nullptr;
                    }
                }
            } else if (token == "LIGHT") {
                // If the type is a light then read light data
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
            bool wantLook = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) && !io.WantCaptureMouse;
            if (wantLook && !mouseCaptured) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                mouseCaptured = true;
                firstMouse = true;
            } else if (!wantLook && mouseCaptured) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                mouseCaptured = false;
            }

            if (mouseCaptured) {
                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);
                if (firstMouse) {
                    lastMouseX = xpos;
                    lastMouseY = ypos;
                    firstMouse = false;
                }
                float xoffset = (float)(xpos - lastMouseX);
                float yoffset = (float)(lastMouseY - ypos);
                lastMouseX = xpos;
                lastMouseY = ypos;
                camera.ProcessMouseMovement(xoffset * -1, yoffset);
            }

            if (!io.WantCaptureKeyboard) {
                glm::vec3 forward = glm::normalize(camera.Front);
                glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
                glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
                float velocity = movementSpeed * deltaTime;
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.Position += forward * velocity;
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.Position -= forward * velocity;
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.Position -= right * velocity;
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.Position += right * velocity;
                if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) camera.Position += worldUp * velocity;
                if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) camera.Position -= worldUp * velocity;
            }
        }
        {
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
			ImGuiID maindock_id = ImGui::GetID("MainDock");
			ImGui::DockSpaceOverViewport(maindock_id, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
            ImGui::PopStyleColor(1);
        }

        // Properties / Object chooser
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
            static int addType = 0; // 0 = Mesh, 1 = Light 2 = Landscape
            ImGui::RadioButton("Mesh", &addType, 0); ImGui::SameLine();
            ImGui::RadioButton("Light", &addType, 1); ImGui::SameLine();
            ImGui::RadioButton("Landscape", &addType, 2);
            ImGui::InputText("Mesh path", newMeshPath, sizeof(newMeshPath));
            if (ImGui::Button("Add Object")) {
                SceneObject o;
                if (addType == 0) {
                    o.type = SceneObject::MeshObj;
                    o.name = std::string("Mesh_") + std::to_string(sceneObjects.size());
                    o.resource = std::string(newMeshPath);
                } else if (addType == 1) {
                    o.type = SceneObject::LightObj;
                    o.name = std::string("Light_") + std::to_string(sceneObjects.size());
                    o.light.Color = glm::vec3(1.0f);
                    o.light.Intensity = 1.0f;
                } else if (addType == 2) {
                    o.type = SceneObject::LandscapeObj;
                    o.name = std::string("Landscape") + std::to_string(sceneObjects.size());
                    o.terrain.Heightmap = nullptr; // Placeholder for heightmap
                    o.terrain.Material = nullptr; // Placeholder for terrain material
                }
                // TODO: Add landscape option for terrain systems
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
                            so.material.DiffusePath = std::string(pathBuf);
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
            // Content Drawer
            ImGui::Begin("Content Drawer", nullptr, ImGuiWindowFlags_DockNodeHost);
            ImGui::Text("Assets in assets/ directory:");
            ImGui::Separator();
            for (const auto &entry : std::filesystem::directory_iterator("assets")) {
                if (entry.is_regular_file()) {
                    const auto &path = entry.path();
                    ImGui::Text("%s", path.filename().string().c_str());
                }
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