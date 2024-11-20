#pragma once
#include "scene.h"

namespace gpr5300
{

class Engine
{
public:
    Engine(Scene* scene);
    void Run();
private:
    void Begin();
    void End();
    Scene* scene_ = nullptr;
    SDL_Window* window_ = nullptr;
    SDL_GLContext glRenderContext_{};
};
    
} // namespace gpr5300
