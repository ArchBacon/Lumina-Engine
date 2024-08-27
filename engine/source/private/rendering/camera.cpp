#include "camera.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace lumina
{
    void Camera::Update(float deltaTime)
    {
        glm::mat4 cameraRotation = GetRotationMatrix();
        position += float3(cameraRotation * float4(velocity * speed, 0.0f) * deltaTime);
    }

    void Camera::ProcessSDLEvent(const SDL_Event& event)
    {
        if (event.type == SDL_KEYDOWN)
        {
            if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_RIGHT))
            {
                if (event.key.keysym.sym == SDLK_w) { velocity.z = -1.0f;}
                if (event.key.keysym.sym == SDLK_s) { velocity.z = 1.0f;}
                if (event.key.keysym.sym == SDLK_a) { velocity.x = -1.0f;}
                if (event.key.keysym.sym == SDLK_d) { velocity.x = 1.0f;}
            }            
        }
        if (event.type == SDL_KEYUP)
        {
            if (event.key.keysym.sym == SDLK_w) { velocity.z = 0.0f;}
            if (event.key.keysym.sym == SDLK_s) { velocity.z = 0.0f;}
            if (event.key.keysym.sym == SDLK_a) { velocity.x = 0.0f;}
            if (event.key.keysym.sym == SDLK_d) { velocity.x = 0.0f;}
        }

        if (event.type == SDL_MOUSEBUTTONDOWN)
        {
            if (event.button.button == SDL_BUTTON_RIGHT)
            {
                SDL_GetMouseState(&lastMousePos.x, &lastMousePos.y);
                SDL_SetRelativeMouseMode(SDL_TRUE);
            }
        }
        if (event.type == SDL_MOUSEBUTTONUP)
        {
            if (event.button.button == SDL_BUTTON_RIGHT)
            {
                SDL_SetRelativeMouseMode(SDL_FALSE);
                SDL_WarpMouseInWindow(nullptr, lastMousePos.x, lastMousePos.y);
                velocity = float3{0.0f};
            }
        }
        if (event.type == SDL_MOUSEMOTION)
        {
            if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_RIGHT))
            {
                yaw += static_cast<float>(event.motion.xrel) / 200.0f;
                pitch -= static_cast<float>(event.motion.yrel) / 200.0f;
            }
        }       
    }

    glm::mat4 Camera::GetViewMatrix() const
    {
        glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 cameraRotation = GetRotationMatrix();
        return glm::inverse(cameraTranslation * cameraRotation);
    }

    glm::mat4 Camera::GetRotationMatrix() const
    {
        glm::quat pitchRotation = glm::angleAxis(pitch, float3{1.0f, 0.0f, 0.0f});
        glm::quat yawRotation = glm::angleAxis(yaw, float3{0.0f, -1.0f, 0.0f,});

        return glm::toMat4(yawRotation) *  glm::toMat4(pitchRotation);
    }    
}
