//
// Created by HUSTLX on 2024/10/22.
//

#ifndef HARDWAREPATHTRACER_CAMERA_H
#define HARDWAREPATHTRACER_CAMERA_H

#include "core/Core.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>


namespace HWPT {
    enum class CameraType : uint8_t {
        None,
        Orthographic,
        Perspective
    };

    class CameraBase {
    public:
        explicit CameraBase(CameraType Type) : m_type(Type) {}

        [[nodiscard]] auto GetCameraType() const {
            return m_type;
        }

    protected:
        CameraType m_type = CameraType::None;
    };

    // TODO
    class OrthographicCamera : public CameraBase {
    public:

    private:

    };

    class PerspectiveCamera : public CameraBase {
    public:
        PerspectiveCamera(const glm::vec3 &Pos, uint Width, uint Height, float Fov,
                          float Near, float Far);

        PerspectiveCamera(const glm::vec3 &Pos, float AspectRatio, float Fov,
                          float Near, float Far);

        void UpdateCameraParams();

        [[nodiscard]] auto GetViewTrans() const -> const glm::mat4 & {
            return m_viewTrans;
        }

        [[nodiscard]] auto GetProjTrans() const -> const glm::mat4 & {
            return m_projTrans;
        }

    private:
        glm::vec3 m_cameraPos;
        inline static glm::vec3 s_cameraDirection = glm::vec3(0.f, 0.f, -1.f);
        inline static glm::vec3 s_up = glm::vec3(0.f, 1.f, 0.f);
        float m_pitch = 0.f, m_yaw = 0.f, m_roll = 0.f;
        float m_fovDegree = 45.f;
        float m_nearPlane = .1f;
        float m_farPlane = 1e3f;
        float m_aspectRatio = 1.f;

        glm::mat4 m_viewTrans{}, m_projTrans{};
        glm::quat m_rotateQuad{};
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_CAMERA_H
