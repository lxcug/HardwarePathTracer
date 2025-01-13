//
// Created by HUSTLX on 2025/1/5.
// Refer: https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR
//

#ifndef HARDWAREPATHTRACER_ACCELERATIONSTRUCTURE_H
#define HARDWAREPATHTRACER_ACCELERATIONSTRUCTURE_H

#include "Core/Core.h"
#include <vector>
#include "core/buffer/ArbitraryBuffer.h"
#include "core/Commands/CommandPool.h"
#include <tuple>


#if !BUILD_RELEASE && !BUILD_SHIPPING
#define PROFILE_AS_BUILD 1
#else
#define PROFILE_AS_BUILD 0
#endif


namespace HWPT
{
    // Collect from Mesh as BLASBuildInput
    struct BLASBuildInput
    {
        std::vector<VkAccelerationStructureGeometryKHR> ASGeometries;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> ASBuildRangeInfos;
        VkBuildAccelerationStructureFlagsKHR Flags{0};
    };

    // The Data Needed to Build TLAS and BLAS
    struct ASBuildData
    {
        std::vector<VkAccelerationStructureGeometryKHR> ASGeometries;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> ASBuildRangeInfos;

        VkAccelerationStructureTypeKHR ASType = VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR;
        VkAccelerationStructureBuildGeometryInfoKHR BuildInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR
        };
        VkAccelerationStructureBuildSizesInfoKHR SizeInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
        };

        auto FinalizeGeometry(
            VkBuildAccelerationStructureFlagsKHR BuildFlags) ->
            VkAccelerationStructureBuildSizesInfoKHR&;

        [[nodiscard]] auto
        MakeInstanceGeometry(uint InstanceCount, VkDeviceAddress InstanceBufferAddress) const ->
            std::tuple<VkAccelerationStructureGeometryKHR,
                       VkAccelerationStructureBuildRangeInfoKHR>;

        void AddGeometry(const VkAccelerationStructureGeometryKHR& Geometry,
                         const VkAccelerationStructureBuildRangeInfoKHR& BuildRange);
    };

    struct Accel
    {
        VkAccelerationStructureKHR AccelHandle = VK_NULL_HANDLE;
        ArbitraryBuffer* Buffer = nullptr;
        VkDeviceAddress Address = 0;

        ~Accel()
        {
            delete Buffer;

            auto Func = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(
                GetVKDevice(), "vkDestroyAccelerationStructureKHR"));
            Func(GetVKDevice(), AccelHandle, nullptr);
        }
    };

    struct ScratchSizeInfo
    {
        VkDeviceSize MaxScratchSize = 0;
        VkDeviceSize TotalScratchSize = 0;
    };

    class ASBuilder
    {
    public:
        ASBuilder()
        {
            this->Init();
        }

        ~ASBuilder();

        void Init();

        void BuildBLAS(std::vector<BLASBuildInput>& BuildInput,
                       VkBuildAccelerationStructureFlagsKHR BuildFlags =
                           VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

        // TODO
        void UpdateBLAS(uint BLASIndex,
                        BLASBuildInput& BuildInput,
                        VkBuildAccelerationStructureFlagsKHR BuildFlags);

        void BuildTLAS(std::vector<VkAccelerationStructureInstanceKHR>& Instances,
                       VkBuildAccelerationStructureFlagsKHR BuildFlags =
                           VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
                       bool Update = false);

        [[nodiscard]] auto GetTLAS() const -> VkAccelerationStructureKHR
        {
            return m_TLAS.AccelHandle;
        }

        [[nodiscard]] auto GetBLASDeviceAddress(uint BLASIndex) const -> VkDeviceAddress;

    protected:
        static auto ComputeScratchAlignedSize(const std::vector<ASBuildData>& BuildData,
                                              uint Alignment) -> ScratchSizeInfo;

        [[nodiscard]] static auto
        GetScratchSize(VkDeviceSize MemBudget,
                       const std::vector<ASBuildData>& BuildData,
                       uint Alignment = 128) -> VkDeviceSize;

        static void GetScratchAddress(VkDeviceSize MemBudget,
                                      std::vector<ASBuildData>& BuildData,
                                      VkDeviceAddress ScratchBufferAddress,
                                      std::vector<VkDeviceAddress>& ScratchAddresses,
                                      uint Alignment);

        static auto ParallelCreateBLAS(VkCommandBuffer CommandBuffer,
                                       std::vector<ASBuildData>& BuildData,
                                       std::vector<Accel>& BLAS,
                                       const std::vector<VkDeviceAddress>& ScratchAddresses,
                                       VkDeviceSize MemBudget) -> bool;

        static auto BuildAccelerationStructures(VkCommandBuffer CommandBuffer,
                                                std::vector<ASBuildData>& BuildData,
                                                std::vector<Accel>& BLAS,
                                                const std::vector<VkDeviceAddress>&
                                                ScratchAddresses,
                                                VkDeviceSize MemBudget,
                                                VkDeviceSize CurrentBudget) -> VkDeviceSize;

    protected:
        std::vector<Accel> m_BLAS;
        Accel m_TLAS;
        CommandPool* m_commandPool = nullptr;
        static inline uint s_currentBLASIndex = 0;

#if PROFILE_AS_BUILD
        static inline VkQueryPool m_queryPool = VK_NULL_HANDLE;
#endif
    };
} // namespace HWPT

#endif //HARDWAREPATHTRACER_ACCELERATIONSTRUCTURE_H
