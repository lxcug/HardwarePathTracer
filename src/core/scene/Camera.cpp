//
// Created by HUSTLX on 2025/1/10.
//

#include "Camera.h"
#include "core/input/Input.h"


namespace HWPT {

    void CameraBase::Tick(float DeltaTime) {
        float MoveSpeed = s_moveSpeed * m_moveSpeedMultiplier * DeltaTime;
        if (Input::IsKeyPressed(KeyCode::W)) {
            m_cameraPos += MoveSpeed * GetForwardDirection();
        }
        if (Input::IsKeyPressed(KeyCode::S)) {
            m_cameraPos -= MoveSpeed * GetForwardDirection();
        }
        if (Input::IsKeyPressed(KeyCode::A)) {
            m_cameraPos -= MoveSpeed * GetRightDirection();
        }
        if (Input::IsKeyPressed(KeyCode::D)) {
            m_cameraPos += MoveSpeed * GetRightDirection();
        }
        if (Input::IsKeyPressed(KeyCode::Q)) {
            m_cameraPos += MoveSpeed * GetUpDirection();
        }
        if (Input::IsKeyPressed(KeyCode::E)) {
            m_cameraPos -= MoveSpeed * GetUpDirection();
        }

        if (Input::IsMouseButtonPressed(MouseCode::ButtonLeft)) {
            float RotateSpeed = s_rotateSpeed * m_rotateSpeedMultiplier * DeltaTime;
            if (m_isFirstTouch) {
                m_lastMousePos = Input::GetMousePosition();
                m_isFirstTouch = false;
            } else {
                glm::vec2 CurrentPos = Input::GetMousePosition();
                glm::vec2 Offset = CurrentPos - m_lastMousePos;
                float yawSign = GetUpDirection().y > 0.f ? 1.f : -1.f;
                m_pitch += Offset.y * RotateSpeed;
                m_yaw -= yawSign * Offset.x * RotateSpeed;
                m_lastMousePos = CurrentPos;
            }
        } else {
            m_isFirstTouch = true;
        }

        UpdateViewMatrix();
    }
}  // namespace HWPT
