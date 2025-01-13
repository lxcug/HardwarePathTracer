//
// Created by HUSTLX on 2025/1/10.
//

#ifndef HARDWAREPATHTRACER_INPUT_H
#define HARDWAREPATHTRACER_INPUT_H

#include "KeyCode.h"
#include "MouseCode.h"
#include "core/application/ImGuiIntegration.h"
#include "glm/glm.hpp"


namespace HWPT {
    class Input {
    public:
        static auto IsKeyPressed(KeyCode Code) -> bool;

        static auto IsMouseButtonPressed(MouseCode Code) -> bool;

        static auto GetMousePosition() -> glm::vec2;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_INPUT_H
