//
// Created by HUSTLX on 2025/1/10.
//

#include "Camera.h"
#include "core/input/Input.h"


namespace HWPT
{
    void CameraBase::UpdateViewMatrix()
    {
        //            m_viewMatrix = glm::lookAt(m_cameraPos, m_cameraPos + GetForwardDirection(),
        //                                       GetUpDirection());
        //            auto camera2world = glm::translate(glm::identity<glm::mat4>(), m_cameraPos) *
        //                                glm::toMat4(GetOrientation());
        //            m_viewMatrix = glm::inverse(camera2world);
        // NOTE: conjugate of quat
        m_viewMatrix = glm::toMat4(glm::conjugate(GetOrientation())) * glm::translate(
            glm::mat4(1.f), -m_cameraPos);
    }

    void CameraBase::Tick(float DeltaTime)
    {
        m_isMoving = false;
        float MoveSpeed = s_moveSpeed * m_moveSpeedMultiplier * DeltaTime;
        float RotateSpeed = s_rotateSpeed * m_rotateSpeedMultiplier * DeltaTime;
        if (Input::IsKeyPressed(KeyCode::LeftControl) || Input::IsKeyPressed(KeyCode::RightControl))
        {
            MoveSpeed *= .1f;
        }
        if (Input::IsKeyPressed(KeyCode::W))
        {
            m_cameraPos += MoveSpeed * GetForwardDirection();
            m_isMoving = true;
        }
        if (Input::IsKeyPressed(KeyCode::S))
        {
            m_cameraPos -= MoveSpeed * GetForwardDirection();
            m_isMoving = true;
        }
        if (Input::IsKeyPressed(KeyCode::A))
        {
            m_cameraPos -= MoveSpeed * GetRightDirection();
            m_isMoving = true;
        }
        if (Input::IsKeyPressed(KeyCode::D))
        {
            m_cameraPos += MoveSpeed * GetRightDirection();
            m_isMoving = true;
        }
        if (Input::IsKeyPressed(KeyCode::Q))
        {
            m_cameraPos -= MoveSpeed * GetUpDirection();
            m_isMoving = true;
        }
        if (Input::IsKeyPressed(KeyCode::E))
        {
            m_cameraPos += MoveSpeed * GetUpDirection();
            m_isMoving = true;
        }

        if (Input::IsMouseButtonPressed(MouseCode::ButtonLeft))
        {
            if (m_isFirstTouch)
            {
                m_lastMousePos = Input::GetMousePosition();
                m_isFirstTouch = false;
            }
            else
            {
                glm::vec2 CurrentPos = Input::GetMousePosition();
                glm::vec2 Offset = CurrentPos - m_lastMousePos;
                float yawSign = GetUpDirection().y > 0.f ? 1.f : -1.f;
                m_pitch -= Offset.y * RotateSpeed;
                m_yaw -= yawSign * Offset.x * RotateSpeed;
                m_lastMousePos = CurrentPos;
                if (std::abs(Offset.x) > 1e-10f || std::abs(Offset.y) > 1e-10f)
                {
                    m_isMoving = true;
                }
            }
        }
        else
        {
            m_isFirstTouch = true;
        }

        if (m_isMoving)
        {
            UpdateViewMatrix();
        }
    }

    void PerspectiveCamera::UpdateProjMatrix()
    {
        // NOTE: Right Handedness + z from Zero to One
        m_projMatrix = glm::perspectiveRH_ZO(glm::radians(m_fov), m_aspectRatio, m_near, m_far);
        m_projMatrix[1][1] *= -1; // NOTE: Flip Y Axis for Vulkan
    }
} // namespace HWPT
