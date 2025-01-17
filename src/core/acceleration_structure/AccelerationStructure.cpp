//
// Created by HUSTLX on 2025/1/5.
//

#include "AccelerationStructure.h"
#include "core/application/VulkanBackendApp.h"
#include "core/RHI.h"
#include "core/Utils.h"


namespace Shadowy {

    void ASBuilder::Init() {
        m_commandPool = new CommandPool(PoolType::Graphics | PoolType::Transient);
#if PROFILE_AS_BUILD
        VkQueryPoolCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        CreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        CreateInfo.queryCount = 2;  // BLAS Build and Compact Time + TLAS Build Time
        vkCreateQueryPool(GetVKDevice(), &CreateInfo, nullptr, &m_queryPool);
#endif
    }

    auto ASBuildData::FinalizeGeometry(
            VkBuildAccelerationStructureFlagsKHR BuildFlags) -> VkAccelerationStructureBuildSizesInfoKHR & {
        Check(!ASGeometries.empty() && "No Geometry added to ASBuildData");
        Check(ASType != VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR && "AS Type not set");

        BuildInfo.type = ASType;
        BuildInfo.flags = BuildFlags;
        BuildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        BuildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
        BuildInfo.dstAccelerationStructure = VK_NULL_HANDLE;
        BuildInfo.geometryCount = ASGeometries.size();
        BuildInfo.pGeometries = ASGeometries.data();
        BuildInfo.ppGeometries = nullptr;
        BuildInfo.scratchData.deviceAddress = 0;

        std::vector<uint> MaxPrimitiveCount(ASBuildRangeInfos.size());
        for (int i = 0; i < MaxPrimitiveCount.size(); i++) {
            MaxPrimitiveCount[i] = ASBuildRangeInfos[i].primitiveCount;
        }

        auto Func = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(
                GetVKDevice(), "vkGetAccelerationStructureBuildSizesKHR"));
        Func(GetVKDevice(),
             VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
             &BuildInfo,
             MaxPrimitiveCount.data(),
             &SizeInfo);
        return SizeInfo;
    }

    auto ASBuildData::MakeInstanceGeometry(uint InstanceCount,
                                           VkDeviceAddress InstanceBufferAddress) const -> std::tuple<VkAccelerationStructureGeometryKHR, VkAccelerationStructureBuildRangeInfoKHR> {
        Check(ASType == VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR &&
              "Instance Geometry Type should be AK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR");

        VkAccelerationStructureGeometryInstancesDataKHR GeometryInstance{
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
        GeometryInstance.data.deviceAddress = InstanceBufferAddress;

        VkAccelerationStructureGeometryKHR Geometry{
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        Geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        Geometry.geometry.instances = GeometryInstance;

        VkAccelerationStructureBuildRangeInfoKHR BuildRange{};
        BuildRange.primitiveCount = InstanceCount;

        return {Geometry, BuildRange};
    }

    void ASBuildData::AddGeometry(const VkAccelerationStructureGeometryKHR &Geometry,
                                  const VkAccelerationStructureBuildRangeInfoKHR &BuildRange) {
        ASGeometries.push_back(Geometry);
        ASBuildRangeInfos.push_back(BuildRange);
    }

    ASBuilder::~ASBuilder() {
        delete m_commandPool;

#if PROFILE_AS_BUILD
        vkDestroyQueryPool(GetVKDevice(), m_queryPool, nullptr);
#endif
    }

    void ASBuilder::BuildBLAS(std::vector<BLASBuildInput> &BuildInput,
                              VkBuildAccelerationStructureFlagsKHR BuildFlags) {
        uint NumBLAS = BuildInput.size();
        std::vector<ASBuildData> BuildData(NumBLAS);
        m_BLAS.resize(NumBLAS);

        VkDeviceSize MaxBuildScratchSize{0};

        // Find MaxBuildScratchSize
        for (int Index = 0; Index < NumBLAS; Index++) {
            BuildData[Index].ASType = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            BuildData[Index].ASGeometries = std::move(BuildInput[Index].ASGeometries);
            BuildData[Index].ASBuildRangeInfos = std::move(BuildInput[Index].ASBuildRangeInfos);

            auto SizeInfo = BuildData[Index].FinalizeGeometry(BuildInput[Index].Flags | BuildFlags);
            MaxBuildScratchSize = std::max(MaxBuildScratchSize, SizeInfo.buildScratchSize);
        }

        VkDeviceSize MemBudget(256'000'000);  // 256MB
        uint MinAlignment = 128;
        VkDeviceSize ScratchSize = GetScratchSize(MemBudget, BuildData, MinAlignment);

        // Create ScratchBuffer Holding Acceleration Structure Addresses
        ArbitraryBuffer ScratchBuffer(ScratchSize,
                                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        std::vector<VkDeviceAddress> ScratchAddress;
        GetScratchAddress(MemBudget,
                          BuildData,
                          RHI::GetBufferDeviceAddress(ScratchBuffer.GetHandle()),
                          ScratchAddress,
                          MinAlignment);


        bool Finished = false;
        bool ShouldCompaction =
                (BuildFlags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR) ==
                VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;

        do {
            {
                VkCommandBuffer CommandBuffer = m_commandPool->BeginCommandBuffer(
                        QueueType::Graphics);
                Finished = ParallelCreateBLAS(CommandBuffer,
                                              BuildData,
                                              m_BLAS,
                                              ScratchAddress,
                                              MemBudget);
                m_commandPool->SubmitCommandBuffer(CommandBuffer, QueueType::Graphics);
#if PROFILE_AS_BUILD
                std::array<uint64_t, 2> Timestamps{};
                VK_CHECK(vkGetQueryPoolResults(GetVKDevice(), m_queryPool, 0, 2,
                                               sizeof(uint64_t) * 2, Timestamps.data(),
                                               sizeof(uint64_t),
                                               VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));
                uint64_t duration = Timestamps[1] - Timestamps[0];
                std::cout << "BLAS build time: " << static_cast<uint32_t>(duration) / 1e6 << "ms\n";
#endif
            }

            if (ShouldCompaction) {
                VkCommandBuffer CommandBuffer = m_commandPool->BeginCommandBuffer(
                        QueueType::Graphics);
                // TODO: Compact BLAS
                m_commandPool->SubmitCommandBuffer(CommandBuffer, QueueType::Graphics);
            }
        } while (!Finished);
    }

    auto ASBuilder::ComputeScratchAlignedSize(const std::vector<ASBuildData> &BuildData,
                                              uint Alignment) -> ScratchSizeInfo {
        ScratchSizeInfo Ret;

        for (auto &BuildData_: BuildData) {
            VkDeviceSize AlignedSize = Utils::Align(BuildData_.SizeInfo.buildScratchSize,
                                                    Alignment);
            Ret.MaxScratchSize = std::max(Ret.MaxScratchSize, AlignedSize);
            Ret.TotalScratchSize += AlignedSize;
        }

        return Ret;
    }

    auto
    ASBuilder::GetScratchSize(VkDeviceSize MemBudget, const std::vector<ASBuildData> &BuildData,
                              uint Alignment) -> VkDeviceSize {
        ScratchSizeInfo SizeInfo = ComputeScratchAlignedSize(BuildData, Alignment);

        if (SizeInfo.TotalScratchSize < MemBudget) {
            return SizeInfo.TotalScratchSize;
        } else {
            // Use More ScratchBuffers when TotalScratchSize > MemBudget
            uint64_t NumScratchBuffers = std::max(static_cast<uint64_t>(1),
                                                  MemBudget / SizeInfo.MaxScratchSize);
            // Num of ScratchBuffers not more than Num of BLAS
            NumScratchBuffers = std::min(NumScratchBuffers, BuildData.size());
            return NumScratchBuffers * SizeInfo.MaxScratchSize;
        }
    }

    void
    ASBuilder::GetScratchAddress(VkDeviceSize MemBudget,
                                 std::vector<ASBuildData> &BuildData,
                                 VkDeviceAddress ScratchBufferAddress,
                                 std::vector<VkDeviceAddress> &ScratchAddresses,
                                 uint Alignment) {
        ScratchSizeInfo SizeInfo = ComputeScratchAlignedSize(BuildData, Alignment);
        VkDeviceSize MaxScratchSize = SizeInfo.MaxScratchSize;
        VkDeviceSize TotalScratchSize = SizeInfo.TotalScratchSize;

        if (TotalScratchSize < MemBudget) {
            VkDeviceAddress Address = 0;
            for (auto &BuildData_: BuildData) {
                ScratchAddresses.push_back(ScratchBufferAddress + Address);
                VkDeviceSize AlignedSize = Utils::Align(BuildData_.SizeInfo.buildScratchSize,
                                                        Alignment);
                Address += AlignedSize;
            }
        } else {
            // Use More ScratchBuffers when TotalScratchSize > MemBudget
            uint64_t NumScratchBuffers = std::max(static_cast<uint64_t>(1),
                                                  MemBudget / SizeInfo.MaxScratchSize);
            // Num of ScratchBuffers not more than Num of BLAS
            NumScratchBuffers = std::min(NumScratchBuffers, BuildData.size());

            VkDeviceAddress Address = 0;
            for (int i = 0; i < NumScratchBuffers; i++) {
                ScratchAddresses.push_back(ScratchBufferAddress + Address);
                Address += MaxScratchSize;
            }
        }
    }

    auto ASBuilder::ParallelCreateBLAS(VkCommandBuffer CommandBuffer,
                                       std::vector<ASBuildData> &BuildData,
                                       std::vector<Accel> &BLAS,
                                       const std::vector<VkDeviceAddress> &ScratchAddresses,
                                       VkDeviceSize MemBudget) -> bool {
        VkDeviceSize MemUsed = 0;

        while (s_currentBLASIndex < BuildData.size() && MemUsed < MemBudget) {
            MemUsed += BuildAccelerationStructures(CommandBuffer,
                                                   BuildData,
                                                   BLAS,
                                                   ScratchAddresses,
                                                   MemBudget,
                                                   MemUsed);
        }

        return s_currentBLASIndex >= BuildData.size();
    }

    auto ASBuilder::BuildAccelerationStructures(VkCommandBuffer CommandBuffer,
                                                std::vector<ASBuildData> &BuildData,
                                                std::vector<Accel> &BLAS,
                                                const std::vector<VkDeviceAddress> &ScratchAddresses,
                                                VkDeviceSize MemBudget,
                                                VkDeviceSize CurrentBudget) -> VkDeviceSize {
        std::vector<VkAccelerationStructureBuildGeometryInfoKHR> CollectedBuildInfos;
        std::vector<VkAccelerationStructureKHR> CollectedAccels;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR *> CollectedRangeInfos;
        CollectedBuildInfos.reserve(BuildData.size());
        CollectedAccels.reserve(BuildData.size());
        CollectedRangeInfos.reserve(BuildData.size());

        VkDeviceSize BudgetUsed = 0;
        while (CollectedBuildInfos.size() < ScratchAddresses.size() &&
               CurrentBudget + BudgetUsed < MemBudget && s_currentBLASIndex < BuildData.size()) {
            // Create BLAS handle for s_currentBLASIndex
            auto &Data = BuildData[s_currentBLASIndex];
            auto &CurrentBLAS = BLAS[s_currentBLASIndex];
            VkAccelerationStructureCreateInfoKHR CreateInfo{
                    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
            CreateInfo.type = Data.ASType;
            CreateInfo.size = Data.SizeInfo.accelerationStructureSize;
            CurrentBLAS.Buffer = new ArbitraryBuffer(CreateInfo.size,
                                                     VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                                                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            CreateInfo.buffer = CurrentBLAS.Buffer->GetHandle();
            auto CreateASFunc = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(
                    GetVKDevice(), "vkCreateAccelerationStructureKHR"));
            CreateASFunc(GetVKDevice(), &CreateInfo, nullptr, &CurrentBLAS.AccelHandle);
            VkAccelerationStructureDeviceAddressInfoKHR DeviceAddressInfo{
                    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
            DeviceAddressInfo.accelerationStructure = CurrentBLAS.AccelHandle;
            auto GetASDeviceAddressFunc = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(
                    GetVKDevice(), "vkGetAccelerationStructureDeviceAddressKHR"));
            CurrentBLAS.Address = GetASDeviceAddressFunc(GetVKDevice(), &DeviceAddressInfo);
            CollectedAccels.push_back(BLAS[s_currentBLASIndex].AccelHandle);

            // Setup BuildInfo
            Data.BuildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            Data.BuildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
            Data.BuildInfo.dstAccelerationStructure = BLAS[s_currentBLASIndex].AccelHandle;
            Data.BuildInfo.scratchData.deviceAddress = ScratchAddresses[s_currentBLASIndex %
                                                                        ScratchAddresses.size()];
            Data.BuildInfo.geometryCount = Data.ASGeometries.size();
            Data.BuildInfo.pGeometries = Data.ASGeometries.data();
            CollectedBuildInfos.push_back(Data.BuildInfo);
            CollectedRangeInfos.push_back(Data.ASBuildRangeInfos.data());

            BudgetUsed += Data.SizeInfo.accelerationStructureSize;
            s_currentBLASIndex++;
        }

#if PROFILE_AS_BUILD
        vkCmdResetQueryPool(CommandBuffer, m_queryPool, 0, 2);
        vkCmdWriteTimestamp(CommandBuffer,
                            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                            m_queryPool, 0);
#endif
        auto BuildASFunc = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(
                GetVKDevice(), "vkCmdBuildAccelerationStructuresKHR"));
        BuildASFunc(CommandBuffer,
                    CollectedBuildInfos.size(),
                    CollectedBuildInfos.data(),
                    CollectedRangeInfos.data());

        // Barrier to ensure proper synchronization after building
        VkMemoryBarrier Barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        Barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        Barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        vkCmdPipelineBarrier(CommandBuffer,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             0, 1, &Barrier, 0, nullptr, 0, nullptr);
#if PROFILE_AS_BUILD
        vkCmdWriteTimestamp(CommandBuffer,
                            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                            m_queryPool, 1);
#endif
        return BudgetUsed;
    }

    void ASBuilder::BuildTLAS(std::vector<VkAccelerationStructureInstanceKHR> &Instances,
                              VkBuildAccelerationStructureFlagsKHR BuildFlags, bool Update) {
        Check(m_TLAS.AccelHandle == VK_NULL_HANDLE || Update);

        uint InstanceCount = Instances.size();
        auto CommandBuffer = m_commandPool->BeginCommandBuffer(QueueType::Graphics);

        // Create Instance Buffer
        ArbitraryBuffer InstanceBuffer(
                Instances.size() * sizeof(VkAccelerationStructureInstanceKHR),
                static_cast<void *>(Instances.data()),
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VkDeviceAddress InstanceBufferAddress = RHI::GetBufferDeviceAddress(
                InstanceBuffer.GetHandle());

        VkMemoryBarrier Barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        Barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        vkCmdPipelineBarrier(CommandBuffer,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             0, 1, &Barrier, 0, nullptr, 0, nullptr);

        // Create ASBuildData from InstanceBuffer
        ASBuildData BuildData;
        BuildData.ASType = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        // Instance -> Geometry
        auto [GeometryInfo, BuildRangeInfo] = BuildData.MakeInstanceGeometry(InstanceCount,
                                                                             InstanceBufferAddress);
        BuildData.AddGeometry(GeometryInfo, BuildRangeInfo);

        auto SizeInfo = BuildData.FinalizeGeometry(BuildFlags);

        VkDeviceSize ScratchSize = Update ? SizeInfo.updateScratchSize : SizeInfo.buildScratchSize;
        ArbitraryBuffer ScratchBuffer(ScratchSize,
                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VkDeviceAddress ScratchBufferAddress = RHI::GetBufferDeviceAddress(
                ScratchBuffer.GetHandle());

        if (Update) {
            BuildData.ASGeometries[0].geometry.instances.data.deviceAddress = InstanceBufferAddress;
            // TODO vkCmdUpdateAS
        } else {
            VkAccelerationStructureCreateInfoKHR CreateInfo{
                    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};


            CreateInfo.type = BuildData.ASType;
            CreateInfo.size = BuildData.SizeInfo.accelerationStructureSize;
            m_TLAS.Buffer = new ArbitraryBuffer(CreateInfo.size,
                                                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            CreateInfo.buffer = m_TLAS.Buffer->GetHandle();
            auto CreateASFunc = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(
                    GetVKDevice(), "vkCreateAccelerationStructureKHR"));
            CreateASFunc(GetVKDevice(), &CreateInfo, nullptr, &m_TLAS.AccelHandle);

#if PROFILE_AS_BUILD
            vkCmdResetQueryPool(CommandBuffer, m_queryPool, 0, 2);
            vkCmdWriteTimestamp(CommandBuffer,
                                VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                                m_queryPool, 0);
#endif
            BuildData.BuildInfo.dstAccelerationStructure = m_TLAS.AccelHandle;
            BuildData.BuildInfo.scratchData.deviceAddress = ScratchBufferAddress;

            auto BuildASFunc = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(
                    GetVKDevice(), "vkCmdBuildAccelerationStructuresKHR"));
            const VkAccelerationStructureBuildRangeInfoKHR *RangeInfo = BuildData.ASBuildRangeInfos.data();
            BuildASFunc(CommandBuffer, 1, &BuildData.BuildInfo, &RangeInfo);

#if PROFILE_AS_BUILD
            vkCmdWriteTimestamp(CommandBuffer,
                                VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                                m_queryPool, 1);
#endif
        }

        m_commandPool->SubmitCommandBuffer(CommandBuffer, QueueType::Graphics);

#if PROFILE_AS_BUILD
        std::array<uint64_t, 2> Timestamps{};
        VK_CHECK(vkGetQueryPoolResults(GetVKDevice(), m_queryPool, 0, 2,
                                       sizeof(uint64_t) * 2, Timestamps.data(),
                                       sizeof(uint64_t),
                                       VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));
        uint64_t duration = Timestamps[1] - Timestamps[0];
        std::cout << "TLAS build time: " << static_cast<uint32_t>(duration) / 1e6 << "ms\n";
#endif
    }

    auto ASBuilder::GetBLASDeviceAddress(uint BLASIndex) const -> VkDeviceAddress {
        Check(BLASIndex < m_BLAS.size());

        VkAccelerationStructureDeviceAddressInfoKHR AddressInfo{
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
        AddressInfo.accelerationStructure = m_BLAS[BLASIndex].AccelHandle;

        auto GetASDeviceAddressFunc = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(
                GetVKDevice(), "vkGetAccelerationStructureDeviceAddressKHR"));
        return GetASDeviceAddressFunc(GetVKDevice(), &AddressInfo);
    }

}  // namespace Shadowy
