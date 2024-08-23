#pragma once
#include "vk_types.hpp"
#include <SDL/SDL_events.h>

namespace lumina
{
    class Camera
    {
    public:
        float3 velocity{};
        float3 position{};

        float pitch{};
        float yaw{};

        glm::mat4 GetViewMatrix();
        glm::mat4 GetRotationmatrix();

        void ProcessSDLEvent(const SDL_Event& event);
        void Update();
    };

}