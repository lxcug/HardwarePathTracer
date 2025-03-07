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
            ImGui::DragFloat("##Z", &Value.z, 0.1f, Min, Max, Format);
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
                float indent = ImGui::GetCursorPos().x;
                UILayer::DrawFloat3Control("Direction", Light.Direction, 1.f,
                                           ValueChangedSignal, 150.f, -1.f, 1.f);
                UILayer::DrawFloat3Control("Color", Light.Color, 1.f, ValueChangedSignal,
                                           150.f, 0.f, 1.f);
                UILayer::DrawInputFloatControl("Intensity", Light.Intensity, 0.f,
                                               ValueChangedSignal, 150.f, 1.f);
                UILayer::DrawSlideFloatControl("Half Sin Angle", Light.HalfSinAngleOrRange, 0.f,
                                               ValueChangedSignal, 150.f, 0.f, 0.1f, "%.3f");
            } else if (Light.Type == LightType::Point) {
                ImGui::Text("Type: Point");
                UILayer::DrawFloat3Control("Position", Light.Position, 0.f,
                                           ValueChangedSignal);
                UILayer::DrawInputFloatControl("Range", Light.HalfSinAngleOrRange, 5.f,
                                               ValueChangedSignal, 150.f, 1.f, "%.1f");
                UILayer::DrawFloat3Control("Color", Light.Color, 1.f, ValueChangedSignal,
                                           150.f, 0.f, 1.f);
                UILayer::DrawInputFloatControl("Intensity", Light.Intensity, 0.f,
                                               ValueChangedSignal, 150.f, 1.f, "%.1f");
                UILayer::DrawSlideFloatControl("Physical Radius", Light.Radius, 0.f,
                                               ValueChangedSignal, 150.f, 1e-6f, 1.f, "%.1f");
            } else if (Light.Type == LightType::Spot) {

            } else if (Light.Type == LightType::Rect) {

            } else if (Light.Type == LightType::Sky) {
                ImGui::Text("Type: Sky");
                UILayer::DrawFloat3Control("Color", Light.Color, 1.f, ValueChangedSignal,
                                           150.f, 0.f, 1.f);
                UILayer::DrawInputFloatControl("Intensity", Light.Intensity, 0.f,
                                               ValueChangedSignal, 150.f, 1.f, "%.1f");
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

    void UILayer::DrawModelInfo(Model *Model, bool *ValueChangedSignal) {
        ImGui::PushID(Model->m_name.c_str());
        if (ImGui::CollapsingHeader(Model->m_name.c_str())) {
            for (auto *Mesh: Model->m_meshes) {
                ImGui::PushID(Mesh->m_name.c_str());
                if (ImGui::CollapsingHeader(Mesh->m_name.c_str())) {
                    uint MaterialIndex = Mesh->m_meshDesc.MaterialIndex;
                    InputMaterial *Material = &Model->m_materials[MaterialIndex];
                    DrawMaterialInfo(Material, ValueChangedSignal);
                }
                ImGui::PopID();
            }
        }
        ImGui::PopID();
    }

    void UILayer::DrawMeshInfo(Model *Model, Mesh *Mesh, bool *ValueChangedSignal) {

    }

    void UILayer::DrawMaterialInfo(InputMaterial *Material, bool *ValueChangedSignal) {
        DrawSlideFloatControl("roughness", Material->roughness, .5f, ValueChangedSignal);
        DrawSlideFloatControl("metallic", Material->metallic, 0.f, ValueChangedSignal);
        DrawFloat3Control("emission", Material->emission, 0.f, ValueChangedSignal);
        DrawSlideFloatControl("transmission", Material->transmission, 0.f, ValueChangedSignal);
        DrawSlideFloatControl("opacity", Material->opacity, 1.f, ValueChangedSignal);
        DrawSlideFloatControl("ior", Material->ior, 1.5f, ValueChangedSignal, 150.f, 1.0001f, 3.f);
        DrawSlideFloatControl("specular", Material->specular, .5f, ValueChangedSignal);
        DrawSlideFloatControl("specular tint", Material->specular_tint, 0.f, ValueChangedSignal);
        DrawSlideFloatControl("clearcoat", Material->clearcoat, 0.f, ValueChangedSignal);
        DrawSlideFloatControl("clearcoat roughness", Material->clearcoat_roughness, 0.f, ValueChangedSignal);
        DrawSlideFloatControl("sheen", Material->sheen, 1.f, ValueChangedSignal);
        DrawFloat3Control("sheen tint", Material->sheen_tint, 1.f, ValueChangedSignal);
        DrawSlideFloatControl("subsurface", Material->subsurface, 10.f, ValueChangedSignal);
        DrawSlideFloatControl("anisotropy", Material->anisotropy, 0.f, ValueChangedSignal);
    }
}
