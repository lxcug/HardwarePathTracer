//
// Created by HUSTLX on 2025/1/13.
//

#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "core/Core.h"


namespace HWPT
{
    class Debugger
    {
    public:
        Debugger()
        {
            Init();
        }

        void Init();

        void SetObjectName(const std::string& ObjectName, uint64_t ObjectHandle, VkDebugReportObjectTypeEXT ObjectType);

        void SetObjectTag(uint64_t Tag, size_t Size, uint64_t ObjectHandle, VkDebugReportObjectTypeEXT ObjectType);

        void BeginRegion(VkCommandBuffer CommandBuffer, const std::string& MarkerName,
                         const glm::vec4& MarkerColor) const;

        void EndRegion(VkCommandBuffer CommandBuffer) const;

        void InsertMaker(VkCommandBuffer CommandBuffer, std::string& MarkerName,
                         const glm::vec4& MarkerColor) const;

    protected:
        PFN_vkDebugMarkerSetObjectNameEXT m_setObjectNameFunc{};
        PFN_vkDebugMarkerSetObjectTagEXT m_setObjectTagFunc{};
        PFN_vkCmdDebugMarkerBeginEXT m_beginMarkerFunc{};
        PFN_vkCmdDebugMarkerEndEXT m_endMarkerFunc{};
        PFN_vkCmdDebugMarkerInsertEXT m_insertMakerFunc{};
    };
} // namespace HWPT

#endif //DEBUGGER_H
