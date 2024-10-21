//
// Created by HUSTLX on 2024/10/18.
//

#include "RasterPass.h"
#include <utility>
#include <array>
#include "core/application/VulkanBackendApp.h"


namespace HWPT {
    RasterPass::RasterPass(std::string PassName, PassFlag Flag, const std::string &VertexSPVPath,
                           const char *VertexEntry, const std::string &FragSPVPath,
                           const char *FragEntry, bool IsBasePass, PrimitiveType InPrimitiveType)
            : RenderPassBase(std::move(PassName), Flag), m_primitiveType(InPrimitiveType),
              m_isBasePass(IsBasePass) {
        m_shaders.VertexShader = new ShaderBase(ShaderType::Vertex, VertexSPVPath, VertexEntry);
        m_shaders.FragmentShader = new ShaderBase(ShaderType::Fragment, FragSPVPath, FragEntry);
    }

    RasterPass::RasterPass(std::string PassName, PassFlag Flag, const std::string &VertexSPVPath,
                           const char *VertexEntry, const std::string &GeometrySPVPath,
                           const char *GeometryEntry, const std::string &FragSPVPath,
                           const char *FragEntry, PrimitiveType InPrimitiveType)
            : RenderPassBase(std::move(PassName), Flag), m_primitiveType(InPrimitiveType) {
        m_shaders.VertexShader = new ShaderBase(ShaderType::Vertex, VertexSPVPath, VertexEntry);
        m_shaders.GeometryShader = new ShaderBase(ShaderType::Geometry, GeometrySPVPath,
                                                  GeometryEntry);
        m_shaders.FragmentShader = new ShaderBase(ShaderType::Fragment, FragSPVPath, FragEntry);
    }

    RasterPass::~RasterPass() {
        delete m_vertexBufferLayout;
        vkDestroyRenderPass(GetVKDevice(), m_renderPass, nullptr);
    }

    void RasterPass::OnRenderPassSetupFinish() {
        Check(m_parameters != nullptr);

        if (m_vertexBufferLayout == nullptr) {
            CreateDefaultVertexBufferLayout();
        }
        CreateRenderPass();
        m_parameters->OnShaderParameterSetFinish();  // CreateDescriptorSets
        CreatePipelineLayout();
        CreateRenderPipeline();
    }

    void RasterPass::BindRenderPass(VkCommandBuffer CommandBuffer) {
        m_parameters->OnRenderPassBegin();
        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0,
                                1, &m_parameters->GetDescriptorSets(), 0, nullptr);
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    }

    void RasterPass::CreateDefaultVertexBufferLayout() {
        m_vertexBufferLayout = new VertexBufferLayout(
                {VertexAttribute(VertexAttributeDataType::Float3, "Pos"),
                 VertexAttribute(VertexAttributeDataType::Float3, "Color"),
                 VertexAttribute(VertexAttributeDataType::Float2, "TexCoord")});
    }

    void RasterPass::CreateRenderPass() {
        VkAttachmentDescription MSAAColorAttachment{};
        MSAAColorAttachment.format = GetVKFormat(TextureFormat::RGBA);
        MSAAColorAttachment.samples = GetVKSampleCount(
                VulkanBackendApp::GetApplication()->GetMSAASampleCount());
        if (m_isBasePass) {
            MSAAColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        } else {
            MSAAColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }
        MSAAColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        MSAAColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        MSAAColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        MSAAColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        MSAAColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;  // For resolve MSAA buffer

        VkAttachmentDescription DepthAttachment{};
        DepthAttachment.format = GetVKFormat(TextureFormat::Depth32);
        DepthAttachment.samples = GetVKSampleCount(
                VulkanBackendApp::GetApplication()->GetMSAASampleCount());
        if (m_isBasePass) {
            DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        } else {
            DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }
        DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription ResolvedColorAttachment{};
        ResolvedColorAttachment.format = GetVKFormat(TextureFormat::RGBA);
        ResolvedColorAttachment.samples = GetVKSampleCount(1u);
        ResolvedColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // Don't Care Load Op Cause MSAAColor will Resolve into ColorAttachment
        ResolvedColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        ResolvedColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ResolvedColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        ResolvedColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ResolvedColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference MSAAColorAttachmentRef{};
        MSAAColorAttachmentRef.attachment = 0;
        MSAAColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference DepthAttachmentRef{};
        DepthAttachmentRef.attachment = 1;
        DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference ResolvedColorAttachmentRef{};
        ResolvedColorAttachmentRef.attachment = 2;
        ResolvedColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription SubPass{};
        SubPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        SubPass.colorAttachmentCount = 1;
        SubPass.pColorAttachments = &MSAAColorAttachmentRef;
        SubPass.pDepthStencilAttachment = &DepthAttachmentRef;
        SubPass.pResolveAttachments = &ResolvedColorAttachmentRef;

        VkSubpassDependency Dependency{};
        Dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        Dependency.dstSubpass = 0;
        Dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        Dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        Dependency.srcAccessMask = 0;
        Dependency.dstAccessMask =
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 3> Attachments = {
                MSAAColorAttachment,
                DepthAttachment,
                ResolvedColorAttachment
        };
        VkRenderPassCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        CreateInfo.attachmentCount = Attachments.size();
        CreateInfo.pAttachments = Attachments.data();
        CreateInfo.subpassCount = 1;
        CreateInfo.pSubpasses = &SubPass;
        CreateInfo.dependencyCount = 1;
        CreateInfo.pDependencies = &Dependency;

        VK_CHECK(vkCreateRenderPass(GetVKDevice(), &CreateInfo, nullptr, &m_renderPass));
    }

    void
    RasterPass::SetVertexBufferLayout(
            const std::initializer_list<VertexAttribute> &VertexAttributes) {
        delete m_vertexBufferLayout;
        m_vertexBufferLayout = new VertexBufferLayout(VertexAttributes);
    }

    void RasterPass::CreateRenderPipeline() {
        VkPipelineShaderStageCreateInfo VertShaderStageInfo{};
        VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        VertShaderStageInfo.module = m_shaders.VertexShader.value()->GetHandle();
        VertShaderStageInfo.pName = m_shaders.VertexShader.value()->GetEntryName();

        VkPipelineShaderStageCreateInfo FragShaderStageInfo{};
        FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        FragShaderStageInfo.module = m_shaders.FragmentShader.value()->GetHandle();
        FragShaderStageInfo.pName = m_shaders.FragmentShader.value()->GetEntryName();

        std::vector<VkPipelineShaderStageCreateInfo> ShaderStages = {
                VertShaderStageInfo, FragShaderStageInfo
        };
        bool HasGeometryStage = m_shaders.GeometryShader.has_value();
        if (HasGeometryStage) {
            VkPipelineShaderStageCreateInfo GeometryShaderStageInfo{};
            GeometryShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            GeometryShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
            GeometryShaderStageInfo.module = m_shaders.GeometryShader.value()->GetHandle();
            GeometryShaderStageInfo.pName = m_shaders.GeometryShader.value()->GetEntryName();
            ShaderStages.push_back(GeometryShaderStageInfo);
        }

        VkPipelineVertexInputStateCreateInfo VertexInputInfo{};
        VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        auto bindingDescription = m_vertexBufferLayout->GetBindingDescription();
        auto attributeDescription = m_vertexBufferLayout->GetAttributeDescriptions();
        VertexInputInfo.vertexBindingDescriptionCount = 1;
        VertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        VertexInputInfo.vertexAttributeDescriptionCount = attributeDescription.size();
        VertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

        VkPipelineInputAssemblyStateCreateInfo InputAssemble{};
        InputAssemble.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        VkPrimitiveTopology PrimitiveTopology = GetVKPrimitiveType(m_primitiveType);
        InputAssemble.topology = PrimitiveTopology;
        InputAssemble.primitiveRestartEnable = VK_FALSE;

        std::vector<VkDynamicState> DynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,
                VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT
        };
        VkPipelineDynamicStateCreateInfo DynamicState{};
        DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        DynamicState.dynamicStateCount = DynamicStates.size();
        DynamicState.pDynamicStates = DynamicStates.data();

        VkPipelineViewportStateCreateInfo ViewportState{};
        ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        ViewportState.viewportCount = 0;

        VkPipelineRasterizationStateCreateInfo Rasterizer{};
        Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        Rasterizer.depthClampEnable = VK_FALSE;
        Rasterizer.rasterizerDiscardEnable = VK_FALSE;
        Rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        Rasterizer.lineWidth = 1.f;
        Rasterizer.cullMode = VK_CULL_MODE_NONE;
        Rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        Rasterizer.depthBiasEnable = VK_FALSE;
        Rasterizer.depthBiasConstantFactor = 0.f;
        Rasterizer.depthBiasClamp = 0.f;
        Rasterizer.depthBiasSlopeFactor = 0.f;

        VkPipelineMultisampleStateCreateInfo Multisampling{};
        Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        Multisampling.sampleShadingEnable = VK_FALSE;
        Multisampling.rasterizationSamples = GetVKSampleCount(
                VulkanBackendApp::GetApplication()->GetMSAASampleCount());
        Multisampling.minSampleShading = 1.f;
        Multisampling.pSampleMask = nullptr;
        Multisampling.alphaToCoverageEnable = VK_FALSE;
        Multisampling.alphaToOneEnable = VK_FALSE;
        Multisampling.sampleShadingEnable = VK_TRUE;
        Multisampling.minSampleShading = .2f;

        VkPipelineColorBlendAttachmentState ColorBlendAttachment{};
        ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        ColorBlendAttachment.blendEnable = VK_FALSE;
        ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo ColorBlending{};
        ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        ColorBlending.logicOpEnable = VK_FALSE;
        ColorBlending.logicOp = VK_LOGIC_OP_COPY;
        ColorBlending.attachmentCount = 1;
        ColorBlending.pAttachments = &ColorBlendAttachment;

        VkPipelineDepthStencilStateCreateInfo DepthStencil{};
        DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        DepthStencil.depthTestEnable = VK_TRUE;
        DepthStencil.depthWriteEnable = VK_TRUE;
        DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        DepthStencil.depthBoundsTestEnable = VK_FALSE;
        DepthStencil.stencilTestEnable = VK_FALSE;
        DepthStencil.front = {};
        DepthStencil.back = {};

        VkGraphicsPipelineCreateInfo PipelineInfo{};
        PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        PipelineInfo.stageCount = ShaderStages.size();
        PipelineInfo.pStages = ShaderStages.data();
        PipelineInfo.pVertexInputState = &VertexInputInfo;
        PipelineInfo.pInputAssemblyState = &InputAssemble;
        PipelineInfo.pViewportState = &ViewportState;
        PipelineInfo.pRasterizationState = &Rasterizer;
        PipelineInfo.pMultisampleState = &Multisampling;
        PipelineInfo.pDepthStencilState = &DepthStencil;
        PipelineInfo.pColorBlendState = &ColorBlending;
        PipelineInfo.pDynamicState = &DynamicState;
        PipelineInfo.layout = m_pipelineLayout;
        PipelineInfo.renderPass = m_renderPass;
        PipelineInfo.subpass = 0;
        PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        PipelineInfo.basePipelineIndex = -1;

        VK_CHECK(vkCreateGraphicsPipelines(GetVKDevice(), VK_NULL_HANDLE, 1, &PipelineInfo,
                                           nullptr, &m_pipeline));
    }

    void RasterPass::Execute(const VertexBuffer &InVertexBuffer) {
        InVertexBuffer.Bind(m_commandBuffer);
        vkCmdDraw(m_commandBuffer, InVertexBuffer.GetVertexCount(), 1, 0, 0);
    }

    void RasterPass::Execute(const StorageBuffer &InStorageBuffer, uint VertexCount) {
        BeginRenderPass(m_commandBuffer);
        VkDeviceSize Offset = 0;
        vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &InStorageBuffer.GetHandle(), &Offset);
        vkCmdDraw(m_commandBuffer, VertexCount, 1, 0, 0);
        EndRenderPass(m_commandBuffer);
    }

    void RasterPass::Execute(const VertexBuffer &InVertexBuffer, const IndexBuffer &InIndexBuffer) {
        BeginRenderPass(m_commandBuffer);
        InVertexBuffer.Bind(m_commandBuffer);
        InIndexBuffer.Bind(m_commandBuffer);
        vkCmdDrawIndexed(m_commandBuffer, InIndexBuffer.GetIndexCount(), 1, 0, 0, 0);
        EndRenderPass(m_commandBuffer);
    }

    void RasterPass::Execute(const Model &InModel) {
        BeginRenderPass(m_commandBuffer);
        InModel.DrawIndexed(m_commandBuffer);
        EndRenderPass(m_commandBuffer);
    }

    void RasterPass::BeginRenderPass(VkCommandBuffer CommandBuffer) {
        auto SwapChain = VulkanBackendApp::GetApplication()->GetSwapChain();
        VkRenderPassBeginInfo RasterPassInfo{};
        RasterPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        RasterPassInfo.renderPass = m_renderPass;
        RasterPassInfo.framebuffer = VulkanBackendApp::GetApplication()->GetCurrentFrameBuffer()->GetHandle();
        RasterPassInfo.renderArea.offset = {0, 0};
        RasterPassInfo.renderArea.extent = SwapChain->GetExtent();
        std::array<VkClearValue, 2> ClearValues{};
        ClearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        ClearValues[1].depthStencil = {1.0f, 0};
        RasterPassInfo.clearValueCount = ClearValues.size();
        RasterPassInfo.pClearValues = ClearValues.data();
        vkCmdBeginRenderPass(CommandBuffer, &RasterPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport Viewport{};
        Viewport.x = 0.f;
        Viewport.y = 0.f;
        Viewport.width = static_cast<float>(SwapChain->GetExtent().width);
        Viewport.height = static_cast<float>(SwapChain->GetExtent().height);
        Viewport.minDepth = 0.f;
        Viewport.maxDepth = 1.f;
        vkCmdSetViewportWithCount(CommandBuffer, 1, &Viewport);
        VkRect2D Scissor{};
        Scissor.offset = {0, 0};
        Scissor.extent = SwapChain->GetExtent();
        vkCmdSetScissorWithCount(CommandBuffer, 1, &Scissor);
    }

    void RasterPass::EndRenderPass(VkCommandBuffer CommandBuffer) {
        vkCmdEndRenderPass(CommandBuffer);
    }
}  // namespace HWPT
