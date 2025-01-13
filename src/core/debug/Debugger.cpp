//
// Created by HUSTLX on 2025/1/13.
//

#include "Debugger.h"
#include "core/application/VulkanBackendApp.h"


namespace HWPT
{
    void Debugger::Init()
    {
        m_setObjectNameFunc = reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(
            vkGetDeviceProcAddr(GetVKDevice(), "vkDebugMarkerSetObjectNameEXT"));
        m_setObjectTagFunc = reinterpret_cast<PFN_vkDebugMarkerSetObjectTagEXT>(
            vkGetDeviceProcAddr(GetVKDevice(), "vkDebugMarkerSetObjectTagEXT"));
        m_beginMarkerFunc = reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>(
            vkGetDeviceProcAddr(GetVKDevice(), "vkCmdDebugMarkerBeginEXT"));
        m_endMarkerFunc = reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>(
            vkGetDeviceProcAddr(GetVKDevice(), "vkCmdDebugMarkerEndEXT"));
        m_insertMakerFunc = reinterpret_cast<PFN_vkCmdDebugMarkerInsertEXT>(
            vkGetDeviceProcAddr(GetVKDevice(), "vkCmdDebugMarkerInsertEXT"));
    }

    void Debugger::SetObjectName(const std::string& ObjectName, uint64_t ObjectHandle,
        VkDebugReportObjectTypeEXT ObjectType)
    {

    }

    void Debugger::SetObjectTag(uint64_t Tag, size_t Size, uint64_t ObjectHandle,
                                VkDebugReportObjectTypeEXT ObjectType)
    {
    }

    void Debugger::BeginRegion(VkCommandBuffer CommandBuffer, const std::string& MarkerName,
                               const glm::vec4& MarkerColor) const
    {
        VkDebugMarkerMarkerInfoEXT MarkerInfo{VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT};
        MarkerInfo.pMarkerName = MarkerName.c_str();
        std::memcpy(MarkerInfo.color, glm::value_ptr(MarkerColor), sizeof(float) * 4);
        m_beginMarkerFunc(CommandBuffer, &MarkerInfo);
    }

    void Debugger::EndRegion(VkCommandBuffer CommandBuffer) const
    {
        m_endMarkerFunc(CommandBuffer);
    }

    void Debugger::InsertMaker(VkCommandBuffer CommandBuffer, std::string& MarkerName,
        const glm::vec4& MarkerColor) const
    {
        VkDebugMarkerMarkerInfoEXT MarkerInfo{VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT};
        MarkerInfo.pMarkerName = MarkerName.c_str();
        std::memcpy(MarkerInfo.color, glm::value_ptr(MarkerColor), sizeof(float) * 4);
        m_insertMakerFunc(CommandBuffer, &MarkerInfo);
    }
} // namespace HWPT
