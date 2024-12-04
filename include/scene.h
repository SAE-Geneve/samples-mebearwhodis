#pragma once

#include <SDL.h>

namespace gpr5300
{
    
    class Scene
    {
    public:
        virtual ~Scene() = default;
        virtual void Begin() = 0;
        virtual void End() = 0;
        virtual void Update(float dt) = 0;
        virtual void DrawImGui() {}
        virtual void OnEvent(const SDL_Event& event) {}
        virtual void UpdateCamera(const float dt) {}

    };

} // namespace gpr5300
