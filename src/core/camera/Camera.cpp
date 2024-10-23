//
// Created by HUSTLX on 2024/10/22.
//

#include "Camera.h"


namespace HWPT {

    PerspectiveCamera::PerspectiveCamera(const glm::vec3 &Pos, uint Width, uint Height, float Fov,
                                         float Near,
                                         float Far)
            : PerspectiveCamera(Pos, static_cast<float>(Width) / static_cast<float>(Height), Fov,
                                Near, Far) {
        UpdateCameraParams();
    }

    PerspectiveCamera::PerspectiveCamera(const glm::vec3 &Pos, float AspectRatio, float Fov,
                                         float Near, float Far)
            : CameraBase(CameraType::Perspective), m_cameraPos(Pos), m_aspectRatio(AspectRatio),
              m_fovDegree(Fov), m_nearPlane(Near), m_farPlane(Far) {
        UpdateCameraParams();
    }

    void PerspectiveCamera::UpdateCameraParams() {
        m_rotateQuad = glm::quat(
                {glm::radians(m_pitch), glm::radians(m_yaw), glm::radians(m_roll)});
        m_viewTrans = glm::mat4_cast(m_rotateQuad) *
                      glm::lookAt(m_cameraPos, m_cameraPos + s_cameraDirection, s_up);
        m_projTrans = glm::perspective(glm::radians(m_fovDegree), m_aspectRatio, m_nearPlane,
                                       m_farPlane);
    }
}  // namespace HWPT
