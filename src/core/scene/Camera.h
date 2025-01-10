//
// Created by HUSTLX on 2025/1/10.
//

#ifndef HARDWAREPATHTRACER_CAMERA_H
#define HARDWAREPATHTRACER_CAMERA_H

#include "core/Core.h"
#include "core/input/KeyCode.h"


namespace HWPT {
    enum class CameraType {
        Orthographic = 0x0,
        Perspective,
        CameraTypeMax = 0x7fffffff
    };

    class CameraBase {
    public:
        CameraBase(CameraType Type, const glm::vec3 &CameraPos, float AspectRatio,
                   float NearPlane, float FarPlane) : m_cameraPos(CameraPos), m_type(Type),
                                                      m_aspectRatio(AspectRatio), m_near(NearPlane),
                                                      m_far(FarPlane) {

        }

        virtual void Init() {
            UpdateViewMatrix();
        }

        void UpdateViewMatrix() {
            // NOTE: conjugate of quat
            m_viewMat = glm::toMat4(glm::conjugate(GetOrientation())) * glm::translate(glm::mat4(1.f), -m_cameraPos);
        }

        // Return if camera is moving
        virtual void Tick(float DeltaTime);

        [[nodiscard]] auto GetOrientation() const -> glm::quat {
            return {glm::vec3(m_pitch, m_yaw, 0.f)};
        }

        [[nodiscard]] auto GetForwardDirection() const {
            return glm::rotate(GetOrientation(), glm::vec3(0.f, 0.f, -1.f));
        }

        [[nodiscard]] auto GetUpDirection() const {
            return glm::rotate(GetOrientation(), glm::vec3(0.f, 1.f, 0.f));
        }

        [[nodiscard]] auto GetRightDirection() const {
            return glm::rotate(GetOrientation(), glm::vec3(1.f, 0.f, 0.f));
        }

        [[nodiscard]] auto GetViewMatrix() const -> glm::mat4 {
            return m_viewMat;
        }

        [[nodiscard]] auto GetProjMatrix() const -> glm::mat4 {
            return m_projMat;
        }

        [[nodiscard]] auto GetCameraPos() const -> glm::vec3 {
            return m_cameraPos;
        }

        void SetCameraMoveMultiplier(float Value) {
            m_moveSpeedMultiplier = Value;
        }

        void SetCameraRotateMultiplier(float Value) {
            m_rotateSpeedMultiplier = Value;
        }

        [[nodiscard]] auto IsMoving() const -> bool {
            return m_isMoving;
        }

    protected:
        CameraType m_type = CameraType::CameraTypeMax;
        glm::vec3 m_cameraPos;
        glm::mat4 m_viewMat = glm::identity<glm::mat4>();
        glm::mat4 m_projMat = glm::identity<glm::mat4>();;
        float m_aspectRatio = 1.f, m_near = 1e-2f, m_far = 1e3f;
        float m_pitch = 0.f, m_yaw = 0.f;
        float m_moveSpeedMultiplier = 1.f, m_rotateSpeedMultiplier = 1.f;
        static inline float s_moveSpeed = 5.f;
        static inline float s_rotateSpeed = 1.f;
        glm::vec2 m_lastMousePos;
        bool m_isFirstTouch = true;
        bool m_isMoving = false;
    };


    class PerspectiveCamera : public CameraBase {
    public:
        PerspectiveCamera(const glm::vec3 &CameraPos, float AspectRatio,
                          float NearPlane, float FarPlane, float FOV)
                : CameraBase(CameraType::Perspective, CameraPos, AspectRatio, NearPlane, FarPlane),
                  m_fov(FOV) {

        }

        void Init() override {
            CameraBase::Init();
            UpdateProjMatrix();
        }

        void UpdateProjMatrix() {
            m_projMat = glm::perspective(glm::radians(m_fov), m_aspectRatio, m_near, m_far);
        }

    protected:
        float m_fov = 45.f;
    };


}  // namespace HWPT


#endif //HARDWAREPATHTRACER_CAMERA_H
