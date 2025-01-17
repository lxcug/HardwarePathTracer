//
// Created by HUSTLX on 2025/1/17.
//

#include <string>
#include "UILayer.h"
#include "imgui_internal.h"


namespace Shadowy {

    void UILayer::DrawFloat3Control(const std::string &Label, glm::vec3 &Value, float ResetValue,
                                    bool *ValueChangeSignal, float ColumnWidth,
                                    float Min, float Max,
                                    const char *Format) {
        ImGui::PushID(Label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, ColumnWidth);
        ImGui::Text("%s", Label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        float LineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 ButtonSize = {LineHeight + 3.0f, LineHeight};

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});

        if (ImGui::Button("X", ButtonSize)) {
            if (ValueChangeSignal) {
                *ValueChangeSignal = true;
            }
            Value.x = ResetValue;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        if (ValueChangeSignal) {
            *ValueChangeSignal |= ImGui::DragFloat("##X", &Value.x, 0.1f, Min, Max, Format);
        } else {
            ImGui::DragFloat("##X", &Value.x, 0.1f, Min, Max, Format);
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});

        if (ImGui::Button("Y", ButtonSize)) {
            if (ValueChangeSignal) {
                *ValueChangeSignal = true;
            }
            Value.y = ResetValue;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        if (ValueChangeSignal) {
            *ValueChangeSignal |= ImGui::DragFloat("##Y", &Value.y, 0.1f, Min, Max, Format);
        } else {
            ImGui::DragFloat("##Y", &Value.y, 0.1f, Min, Max, Format);
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
        if (ImGui::Button("Z", ButtonSize)) {
            if (ValueChangeSignal) {
                *ValueChangeSignal = true;
            }
            Value.z = ResetValue;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        if (ValueChangeSignal) {
            *ValueChangeSignal |= ImGui::DragFloat("##Z", &Value.z, 0.1f, Min, Max, Format);
        } else {
            *ValueChangeSignal |= ImGui::DragFloat("##Z", &Value.z, 0.1f, Min, Max, Format);
        }
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();
    }

    void UILayer::DrawInputFloatControl(const std::string &Label, float &Value, float ResetValue,
                                        bool *ValueChangeSignal, float ColumnWidth, float Step,
                                        const char *Format) {
        ImGui::PushID(Label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, ColumnWidth);
        ImGui::Text("%s", Label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        float LineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 ButtonSize = {LineHeight + 3.0f, LineHeight};

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.125, 0.074, 0.400, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.188, 0.055, 0.929, 1.f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.157, 0., 1., 1.0f});

        if (ImGui::Button(("X"), ButtonSize)) {
            if (ValueChangeSignal) {
                *ValueChangeSignal = true;
            }
            Value = ResetValue;
        }

        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        if (ValueChangeSignal) {
            *ValueChangeSignal |= ImGui::InputFloat("##X", &Value, Step, 10 * Step, Format);
        } else {
            ImGui::InputFloat("##X", &Value, Step, 10 * Step, Format);
        }
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();
    }

    void UILayer::DrawSlideFloatControl(const std::string &Label, float &Value, float ResetValue,
                                        bool *ValueChangeSignal, float ColumnWidth, float Min,
                                        float Max, const char *Format) {
        ImGui::PushID(Label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, ColumnWidth);
        ImGui::Text("%s", Label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        float LineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 ButtonSize = {LineHeight + 3.0f, LineHeight};

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.125, 0.074, 0.400, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.188, 0.055, 0.929, 1.f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.157, 0., 1., 1.0f});

        if (ImGui::Button(("X"), ButtonSize)) {
            if (ValueChangeSignal) {
                *ValueChangeSignal = true;
            }
            Value = ResetValue;
        }

        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        if (ValueChangeSignal) {
            *ValueChangeSignal |= ImGui::SliderFloat("##X", &Value, Min, Max, Format);
        } else {
            ImGui::SliderFloat("##X", &Value, Min, Max, Format);
        }
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();
    }

    void
    UILayer::DrawLightInfo(const std::string &FolderName, Light &Light, bool *ValueChangedSignal) {
        ImGui::PushID(FolderName.c_str());
        if (ImGui::CollapsingHeader(FolderName.c_str())) {
            if (Light.Type == LightType::Directional) {
                ImGui::Text("Type: Directional");
                UILayer::DrawFloat3Control("Direction", Light.Direction, 1.f,
                                           ValueChangedSignal);
                UILayer::DrawFloat3Control("Color", Light.Color, 1.f, ValueChangedSignal,
                                           150.f, 0.f, 1.f);
                UILayer::DrawInputFloatControl("Intensity", Light.Intensity, 0.f,
                                               ValueChangedSignal, 150.f, 1.f);
            } else if (Light.Type == LightType::Point) {
                ImGui::Text("Type: Point");
                UILayer::DrawFloat3Control("Position", Light.Position, 0.f,
                                           ValueChangedSignal);
                UILayer::DrawInputFloatControl("Radius", Light.Radius, 2.f,
                                               ValueChangedSignal, 150.f, .5f, "%.1f");
                UILayer::DrawFloat3Control("Color", Light.Color, 1.f, ValueChangedSignal,
                                           150.f, 0.f, 1.f);
                UILayer::DrawInputFloatControl("Intensity", Light.Intensity, 0.f,
                                               ValueChangedSignal, 150.f, 1.f, "%.1f");
            } else if (Light.Type == LightType::Spot) {

            } else if (Light.Type == LightType::Rect) {

            }
        }
        ImGui::PopID();
    }

    void UILayer::DrawLights(std::vector<Light> &Lights, bool *ValueChangedSignal) {
        for (int idx = 0; idx < Lights.size(); idx++) {
            Light &Light = Lights[idx];
            std::string FolderName = "Light" + std::to_string(idx);
            UILayer::DrawLightInfo(FolderName, Light, ValueChangedSignal);
        }
    }

}
