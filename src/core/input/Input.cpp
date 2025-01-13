//
// Created by HUSTLX on 2025/1/10.
//

#include "Input.h"
#include "core/application/VulkanBackendApp.h"


namespace HWPT
{
    auto Input::IsKeyPressed(KeyCode Code) -> bool
    {
        GLFWwindow* Window = VulkanBackendApp::GetApplication()->GetWindow();
        return glfwGetKey(Window, static_cast<int>(Code)) == GLFW_PRESS;
    }

    auto Input::IsMouseButtonPressed(MouseCode Code) -> bool
    {
        GLFWwindow* Window = VulkanBackendApp::GetApplication()->GetWindow();
        return glfwGetMouseButton(Window, static_cast<int>(Code)) == GLFW_PRESS;
    }

    auto Input::GetMousePosition() -> glm::vec2
    {
        GLFWwindow* Window = VulkanBackendApp::GetApplication()->GetWindow();
        double xPos, yPos;
        glfwGetCursorPos(Window, &xPos, &yPos);
        return {static_cast<float>(xPos), static_cast<float>(yPos)};
    }
} // namespace HWPT
