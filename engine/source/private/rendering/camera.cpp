#include "camera.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace lumina
{
    void Camera::Update()
    {
        glm::mat4 cameraRotation = GetRotationmatrix();
        position += float3(cameraRotation * float4(velocity * 0.5f, 0.0f));
    }

    void Camera::ProcessSDLEvent(const SDL_Event& event)
    {
        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_w) { velocity.z = -1.0f;}
            if (event.key.keysym.sym == SDLK_s) { velocity.z = 1.0f;}
            if (event.key.keysym.sym == SDLK_a) { velocity.x = -1.0f;}
            if (event.key.keysym.sym == SDLK_d) { velocity.x = 1.0f;}
        }
        if (event.type == SDL_KEYUP)
        {
            if (event.key.keysym.sym == SDLK_w) { velocity.z = 0.0f;}
            if (event.key.keysym.sym == SDLK_s) { velocity.z = 0.0f;}
            if (event.key.keysym.sym == SDLK_a) { velocity.x = 0.0f;}
            if (event.key.keysym.sym == SDLK_d) { velocity.x = 0.0f;}
        }

        if (event.type == SDL_MOUSEMOTION)
        {
            yaw += static_cast<float>(event.motion.xrel) / 200.0f;
            pitch -= static_cast<float>(event.motion.yrel) / 200.0f;
        }
    }

    glm::mat4 Camera::GetViewMatrix()
    {
        glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 cameraRotation = GetRotationmatrix();
        return glm::inverse(cameraTranslation * cameraRotation);
    }

    glm::mat4 Camera::GetRotationmatrix()
    {
        glm::quat pitchRotation = glm::angleAxis(pitch, float3{1.0f, 0.0f, 0.0f});
        glm::quat yawRotation = glm::angleAxis(yaw, float3{0.0f, -1.0f, 0.0f,});

        return glm::toMat4(yawRotation) *  glm::toMat4(pitchRotation);
    }    
}
