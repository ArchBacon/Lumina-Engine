#pragma once
#include "vk_types.hpp"

#include <SDL/SDL_events.h>

namespace lumina
{
    class Camera
    {
    public:
        float3 velocity {};
        float3 position {};
        float speed {20.0f};

        float pitch {};
        float yaw {};

        glm::mat4 GetViewMatrix() const;
        glm::mat4 GetRotationMatrix() const;

        void ProcessSDLEvent(const SDL_Event& event);
        void Update(float deltaTime);

    private:
        int2 lastMousePos {};
    };

} // namespace lumina
