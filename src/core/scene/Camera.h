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

        void UpdateViewMatrix();

        // Return if camera is moving
        virtual void Tick(float DeltaTime);

        void SetCameraPosition(const glm::vec3 &Pos) {
            m_cameraPos = Pos;
            UpdateViewMatrix();
        }

        void OnMouseScroll(float XOffset, float YOffset);

        void SetCameraRotation(float Pitch, float Yaw) {
            m_pitch = Pitch;
            m_yaw = Yaw;
            UpdateViewMatrix();
        }

        [[nodiscard]] auto GetOrientation() const -> glm::quat {
            return {glm::vec3(m_pitch, m_yaw, 0.f)};
        }

        [[nodiscard]] auto GetForwardDirection() const -> glm::vec3 {
            return glm::rotate(GetOrientation(), glm::vec3(0.f, 0.f, -1.f));
        }

        [[nodiscard]] auto GetUpDirection() const -> glm::vec3 {
            return glm::rotate(GetOrientation(), glm::vec3(0.f, 1.f, 0.f));
        }

        [[nodiscard]] auto GetRightDirection() const -> glm::vec3 {
            return glm::rotate(GetOrientation(), glm::vec3(1.f, 0.f, 0.f));
        }

        [[nodiscard]] auto GetViewMatrix() const -> glm::mat4 {
            return m_viewMatrix;
        }

        [[nodiscard]] auto GetProjMatrix() const -> glm::mat4 {
            return m_projMatrix;
        }

        [[nodiscard]] auto GetCameraPos() -> glm::vec3 & {
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

        [[nodiscard]] auto GetPitch() const -> float {
            return m_pitch;
        }

        [[nodiscard]] auto GetYaw() const -> float {
            return m_yaw;
        }

        virtual void OnWindowResize(uint Width, uint Height) {
            m_aspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
        }

        auto GetCameraMoveSpeed() -> float & {
            return m_moveSpeedMultiplier;
        }

        auto GetCameraRotateSpeed() -> float & {
            return m_rotateSpeedMultiplier;
        }

        auto GetCameraScrollSpeed() -> float & {
            return m_scrollSpeedMultiplier;
        }

    protected:
        CameraType m_type = CameraType::CameraTypeMax;
        glm::vec3 m_cameraPos;
        glm::mat4 m_viewMatrix = glm::identity<glm::mat4>();
        glm::mat4 m_projMatrix = glm::identity<glm::mat4>();;
        float m_aspectRatio = 1.f, m_near = 1e-1f, m_far = 1e3f;
        float m_pitch = 0.f, m_yaw = 0.f;
        float m_moveSpeedMultiplier = 1.f, m_rotateSpeedMultiplier = 1.f;
        float m_scrollSpeedMultiplier = 1.f;
        static inline float s_moveSpeed = 5.f;
        static inline float s_rotateSpeed = .5f;
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

        void OnWindowResize(uint Width, uint Height) override {
            CameraBase::OnWindowResize(Width, Height);
            UpdateProjMatrix();
        }

        void UpdateProjMatrix();

        auto GetFOV() -> float & {
            return m_fov;
        }

    protected:
        float m_fov = 45.f;
    };
} // namespace HWPT


#endif //HARDWAREPATHTRACER_CAMERA_H
