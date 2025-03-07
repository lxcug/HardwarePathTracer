//
// Created by HUSTLX on 2025/1/17.
//

#ifndef HARDWAREPATHTRACER_UILAYER_H
#define HARDWAREPATHTRACER_UILAYER_H

#include "core/Core.h"
#include "imgui.h"
#include "host_device_shared/Light.h"
#include <vector>
#include "core/scene/Model.h"


namespace Shadowy {
    class UILayer {
    public:
        static void
        DrawFloat3Control(const std::string &Label, glm::vec3 &Value, float ResetValue = 0.f,
                          bool *ValueChangeSignal = nullptr, float ColumnWidth = 150.f,
                          float Min = 0.f, float Max = 1.f,
                          const char *Format = "%.2f");

        static void
        DrawInputFloatControl(const std::string &Label, float &Value, float ResetValue = 0.f,
                              bool *ValueChangeSignal = nullptr, float ColumnWidth = 150.f,
                              float Step = 1.f, const char *Format = "%.2f");

        static void
        DrawSlideFloatControl(const std::string &Label, float &Value, float ResetValue = 0.f,
                              bool *ValueChangeSignal = nullptr, float ColumnWidth = 150.f,
                              float Min = 0.f, float Max = 1.f, const char *Format = "%.2f");

        static void
        DrawLightInfo(const std::string &FolderName, Light &Light,
                      bool *ValueChangedSignal = nullptr);

        static void DrawModelInfo(Model* Model, bool *ValueChangedSignal = nullptr);

        static void DrawMeshInfo(Model* Model, Mesh* Mesh, bool *ValueChangedSignal = nullptr);

        static void DrawMaterialInfo(InputMaterial* Material, bool *ValueChangedSignal = nullptr);

        static void DrawLights(std::vector<Light> &Lights, bool *ValueChangedSignal = nullptr);
    };
}  // namespace Shadowy

#endif //HARDWAREPATHTRACER_UILAYER_H
