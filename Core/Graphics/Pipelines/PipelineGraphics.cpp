#include "PipelineGraphics.hpp"

#include "Files.hpp"
#include "Graphics.hpp"
#include "Log.hpp"

#include "config.h"

namespace MapleLeaf {
const std::vector<VkDynamicState> DYNAMIC_STATES = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH};

PipelineGraphics::PipelineGraphics(Stage stage, std::vector<std::filesystem::path> shaderStages, std::vector<Shader::VertexInput> vertexInputs,
                                   std::vector<Shader::Define> defines, Mode mode, Depth depth, VkPrimitiveTopology topology,
                                   VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace, bool pushDescriptors)
    : stage(stage)
    , shaderStages(std::move(shaderStages))
    , vertexInputs(std::move(vertexInputs))
    , defines(defines)
    , mode(mode)
    , depth(depth)
    , topology(topology)
    , polygonMode(polygonMode)
    , cullMode(cullMode)
    , frontFace(frontFace)
    , pushDescriptors(pushDescriptors)
    , shader(std::make_unique<Shader>())
    , dynamicStates(DYNAMIC_STATES)
    , pipelineBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
{
#ifdef MAPLELEAF_PIPELINE_DEBUG
    auto debugStart = Time::Now();
#endif

    std::sort(this->vertexInputs.begin(), this->vertexInputs.end());
    CreateShaderProgram();
    CreateDescriptorLayout();
    CreateDescriptorPool();
    CreatePipelineLayout();
    CreateAttributes();

    switch (mode) {
    case Mode::Polygon: CreatePipelinePolygon(); break;
    case Mode::MRT: CreatePipelineMrt(); break;
    case Mode::Imgui: CreatePipelineImgui(); break;
    case Mode::Stereo: CreatePipelineStereo(); break;
    case Mode::StereoMRT: CreatePipelineStereoMRT(); break;
    default: throw std::runtime_error("Unknown pipeline mode");
    }

#ifdef MAPLELEAF_PIPELINE_DEBUG
    Log::Out("Pipeline Graphics ", this->shaderStages.back(), " loaded in ", (Time::Now() - debugStart).AsMilliseconds<float>(), "ms\n");
#endif
}

PipelineGraphics::~PipelineGraphics()
{
    auto logicalDevice = Graphics::Get()->GetLogicalDevice();
    auto graphicsQueue = logicalDevice->GetSubmitGraphicsQueue();
    Graphics::CheckVk(vkQueueWaitIdle(graphicsQueue));

    for (const auto& shaderModule : modules) vkDestroyShaderModule(*logicalDevice, shaderModule, nullptr);

    vkDestroyDescriptorPool(*logicalDevice, descriptorPool, nullptr);
    vkDestroyPipeline(*logicalDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(*logicalDevice, pipelineLayout, nullptr);

    for (auto& [setIndex, descriptorSetLayout] : descriptorSetNormalLayouts)
        vkDestroyDescriptorSetLayout(*logicalDevice, descriptorSetLayout, nullptr);
    for (auto& [setIndex, descriptorSetLayout] : descriptorSetBindlessLayouts)
        vkDestroyDescriptorSetLayout(*logicalDevice, descriptorSetLayout, nullptr);
}

const ImageDepth* PipelineGraphics::GetDepthStencil(const std::optional<uint32_t>& stage) const
{
    return Graphics::Get()->GetRenderStage(stage ? *stage : this->stage.first)->GetDepthStencil();
}

const Image2d* PipelineGraphics::GetImage(uint32_t index, const std::optional<uint32_t>& stage) const
{
    return Graphics::Get()->GetRenderStage(stage ? *stage : this->stage.first)->GetFramebuffers()->GetAttachment(index);
}

RenderArea PipelineGraphics::GetRenderArea(const std::optional<uint32_t>& stage) const
{
    return Graphics::Get()->GetRenderStage(stage ? *stage : this->stage.first)->GetRenderArea();
}

void PipelineGraphics::CreateShaderProgram()
{
    std::stringstream defineBlock;
    for (const auto& [defineName, defineValue] : defines) defineBlock << "#define " << defineName << " " << defineValue << '\n';

    for (const auto& shaderStage : shaderStages) {
        auto fileLoaded = Files::Get()->Read(shaderStage);

        if (!fileLoaded) throw std::runtime_error("Could not create pipeline, missing shader stage");

        auto stageFlag    = Shader::GetShaderStage(shaderStage);
        auto shaderModule = shader->CreateShaderModule(shaderStage, *fileLoaded, defineBlock.str(), stageFlag);

        VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {};
        pipelineShaderStageCreateInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipelineShaderStageCreateInfo.stage                           = stageFlag;
        pipelineShaderStageCreateInfo.module                          = shaderModule;
        pipelineShaderStageCreateInfo.pName                           = "main";
        stages.emplace_back(pipelineShaderStageCreateInfo);
        modules.emplace_back(shaderModule);
    }

    shader->CreateReflection();
}

void PipelineGraphics::CreateDescriptorLayout()
{
    const auto& descriptorSetLayoutBindings = shader->GetDescriptorSetLayouts();
    const auto& descriptorSetInfos          = shader->GetDescriptorSetInfos();

    for (const auto& [setIndex, LayoutBindingsForSet] : descriptorSetLayoutBindings) {
        const auto& descriptorSetInfo = descriptorSetInfos.at(setIndex);

        if (descriptorSetInfo.IsRunTimeArray)
            CreateBindlessDescriptorLayout(setIndex, LayoutBindingsForSet);
        else
            CreateNormalDescriptorLayout(setIndex, LayoutBindingsForSet);
    }
}

void PipelineGraphics::CreateBindlessDescriptorLayout(uint32_t setIndex, const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetLayoutBindings)
{
    auto logicalDevice = Graphics::Get()->GetLogicalDevice();

    if (descriptorSetBindlessLayouts.count(setIndex) == 0)
        descriptorSetBindlessLayouts[setIndex] = VK_NULL_HANDLE;
    else
        Log::Error("This Bindless descriptorSetLayout have exist, Muliple create setLayout!");

    uint32_t bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());

    VkDescriptorBindingFlagsEXT flag = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
                                       VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;
    std::vector<VkDescriptorBindingFlagsEXT> flags(bindingCount, flag);

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlags{};
    bindingFlags.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    bindingFlags.bindingCount  = bindingCount;
    bindingFlags.pBindingFlags = flags.data();

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.flags =
        pushDescriptors ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR | VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT
                        : VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
    descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
    descriptorSetLayoutCreateInfo.pBindings    = descriptorSetLayoutBindings.data();
    descriptorSetLayoutCreateInfo.pNext        = &bindingFlags;

    Graphics::CheckVk(vkCreateDescriptorSetLayout(*logicalDevice, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetBindlessLayouts[setIndex]));
}

void PipelineGraphics::CreateNormalDescriptorLayout(uint32_t setIndex, const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetLayoutBindings)
{
    auto logicalDevice = Graphics::Get()->GetLogicalDevice();

    if (descriptorSetNormalLayouts.count(setIndex) == 0)
        descriptorSetNormalLayouts[setIndex] = VK_NULL_HANDLE;
    else
        Log::Error("This Normal descriptorSetLayout have exist, Muliple create setLayout!");

    bool containInputAttachment = false;
    for (const auto& descriptorSetLayoutBinding : descriptorSetLayoutBindings) {
        if (descriptorSetLayoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {
            containInputAttachment = true;
            break;
        }
    }

    uint32_t bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());

    VkDescriptorBindingFlags              flag = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    std::vector<VkDescriptorBindingFlags> flags(bindingCount, flag);

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlags{};
    bindingFlags.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    bindingFlags.bindingCount  = bindingCount;
    bindingFlags.pBindingFlags = flags.data();

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.flags                           = pushDescriptors ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR : 0;
    descriptorSetLayoutCreateInfo.flags |= containInputAttachment ? 0 : VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
    descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
    descriptorSetLayoutCreateInfo.pBindings    = descriptorSetLayoutBindings.data();
    descriptorSetLayoutCreateInfo.pNext        = containInputAttachment ? nullptr : &bindingFlags;

    Graphics::CheckVk(vkCreateDescriptorSetLayout(*logicalDevice, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetNormalLayouts[setIndex]));
}

void PipelineGraphics::CreateDescriptorPool()
{
    auto logicalDevice = Graphics::Get()->GetLogicalDevice();

    auto& descriptorPools = shader->GetDescriptorPools();

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    descriptorPoolCreateInfo.maxSets       = 8192;   // 16384;
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPools.size());
    descriptorPoolCreateInfo.pPoolSizes    = descriptorPools.data();
    Graphics::CheckVk(vkCreateDescriptorPool(*logicalDevice, &descriptorPoolCreateInfo, nullptr, &descriptorPool));
}

void PipelineGraphics::CreatePipelineLayout()
{
    auto logicalDevice = Graphics::Get()->GetLogicalDevice();

    auto pushConstantRanges = shader->GetPushConstantRanges();

    std::vector<VkDescriptorSetLayout> descriptorSetLayoutsData;
    for (const auto& [setIndex, descriptorSetLayout] : descriptorSetNormalLayouts) descriptorSetLayoutsData.push_back(descriptorSetLayout);
    for (const auto& [setIndex, descriptorSetLayout] : descriptorSetBindlessLayouts) descriptorSetLayoutsData.push_back(descriptorSetLayout);


    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount             = descriptorSetLayoutsData.size();
    pipelineLayoutCreateInfo.pSetLayouts                = descriptorSetLayoutsData.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount     = static_cast<uint32_t>(pushConstantRanges.size());
    pipelineLayoutCreateInfo.pPushConstantRanges        = pushConstantRanges.data();
    Graphics::CheckVk(vkCreatePipelineLayout(*logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
}

void PipelineGraphics::CreateAttributes()
{
    auto physicalDevice = Graphics::Get()->GetPhysicalDevice();
    auto logicalDevice  = Graphics::Get()->GetLogicalDevice();

    if (polygonMode == VK_POLYGON_MODE_LINE && !logicalDevice->GetEnabledFeatures().fillModeNonSolid) {
        throw std::runtime_error("Cannot create graphics pipeline with line polygon mode when logical device does not support non solid fills.");
    }

    inputAssemblyState.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology               = topology;
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    rasterizationState.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.depthClampEnable        = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.polygonMode             = polygonMode;
    rasterizationState.cullMode                = cullMode;
    rasterizationState.frontFace               = frontFace;
    rasterizationState.depthBiasEnable         = VK_FALSE;
    // rasterizationState.depthBiasConstantFactor = 0.0f;
    // rasterizationState.depthBiasClamp = 0.0f;
    // rasterizationState.depthBiasSlopeFactor = 0.0f;
    rasterizationState.lineWidth = 1.0f;

    blendAttachmentStates[0].blendEnable         = VK_TRUE;
    blendAttachmentStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachmentStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentStates[0].colorBlendOp        = VK_BLEND_OP_ADD;
    blendAttachmentStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachmentStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
    blendAttachmentStates[0].alphaBlendOp        = VK_BLEND_OP_MAX;
    blendAttachmentStates[0].colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    colorBlendState.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.logicOpEnable     = VK_FALSE;
    colorBlendState.logicOp           = VK_LOGIC_OP_COPY;
    colorBlendState.attachmentCount   = static_cast<uint32_t>(blendAttachmentStates.size());
    colorBlendState.pAttachments      = blendAttachmentStates.data();
    colorBlendState.blendConstants[0] = 0.0f;
    colorBlendState.blendConstants[1] = 0.0f;
    colorBlendState.blendConstants[2] = 0.0f;
    colorBlendState.blendConstants[3] = 0.0f;

    depthStencilState.sType          = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilState.front          = depthStencilState.back;

    switch (depth) {
    case Depth::None:
        depthStencilState.depthTestEnable  = VK_FALSE;
        depthStencilState.depthWriteEnable = VK_FALSE;
        break;
    case Depth::Read:
        depthStencilState.depthTestEnable  = VK_TRUE;
        depthStencilState.depthWriteEnable = VK_FALSE;
        break;
    case Depth::Write:
        depthStencilState.depthTestEnable  = VK_FALSE;
        depthStencilState.depthWriteEnable = VK_TRUE;
        break;
    case Depth::ReadWrite:
        depthStencilState.depthTestEnable  = VK_TRUE;
        depthStencilState.depthWriteEnable = VK_TRUE;
        break;
    }

    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    auto renderStage  = Graphics::Get()->GetRenderStage(stage.first);
    bool multisampled = renderStage->IsMultisampled(stage.second);

    multisampleState.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = multisampled ? physicalDevice->GetMsaaSamples() : VK_SAMPLE_COUNT_1_BIT;
    multisampleState.sampleShadingEnable  = VK_FALSE;

    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates    = dynamicStates.data();

    tessellationState.sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellationState.patchControlPoints = 3;
}

void PipelineGraphics::CreatePipeline()
{
    auto logicalDevice = Graphics::Get()->GetLogicalDevice();
    auto pipelineCache = Graphics::Get()->GetPipelineCache();
    auto renderStage   = Graphics::Get()->GetRenderStage(stage.first);

    std::vector<VkVertexInputBindingDescription>   bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    uint32_t                                       lastAttribute = 0;

    for (const auto& vertexInput : vertexInputs) {
        for (const auto& binding : vertexInput.GetBindingDescriptions()) {
            bindingDescriptions.emplace_back(binding);
        }
        for (const auto& attribute : vertexInput.GetAttributeDescriptions()) {
            auto& newAttribute = attributeDescriptions.emplace_back(attribute);
            newAttribute.location += lastAttribute;
        }
        if (!vertexInput.GetAttributeDescriptions().empty()) lastAttribute = attributeDescriptions.back().location + 1;
    }

    vertexInputStateCreateInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputStateCreateInfo.pVertexBindingDescriptions      = bindingDescriptions.data();
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputStateCreateInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount                   = static_cast<uint32_t>(stages.size());
    pipelineCreateInfo.pStages                      = stages.data();

    pipelineCreateInfo.pVertexInputState   = &vertexInputStateCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pTessellationState  = &tessellationState;
    pipelineCreateInfo.pViewportState      = &viewportState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pMultisampleState   = &multisampleState;
    pipelineCreateInfo.pDepthStencilState  = &depthStencilState;
    pipelineCreateInfo.pColorBlendState    = &colorBlendState;
    pipelineCreateInfo.pDynamicState       = &dynamicState;

    pipelineCreateInfo.layout             = pipelineLayout;
    pipelineCreateInfo.renderPass         = *renderStage->GetRenderpass();
    pipelineCreateInfo.subpass            = stage.second;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex  = -1;
    Graphics::CheckVk(vkCreateGraphicsPipelines(*logicalDevice, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
}

void PipelineGraphics::CreatePipelinePolygon()
{
    CreatePipeline();
}

void PipelineGraphics::CreatePipelineMrt()
{
    auto renderStage        = Graphics::Get()->GetRenderStage(stage.first);
    auto attachmentBindings = renderStage->GetOutputAttachmentBindings(stage.second);

    std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
    blendAttachmentStates.reserve(attachmentBindings.size());

    for (const auto& attachmentBinding : attachmentBindings) {
        VkPipelineColorBlendAttachmentState blendAttachmentState = {};
        blendAttachmentState.blendEnable                         = renderStage->GetAttachment(attachmentBinding)->EnableBlend() ? VK_TRUE : VK_FALSE;
        blendAttachmentState.srcColorBlendFactor                 = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachmentState.dstColorBlendFactor                 = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState.colorBlendOp                        = VK_BLEND_OP_ADD;
        blendAttachmentState.srcAlphaBlendFactor                 = VK_BLEND_FACTOR_ONE;
        blendAttachmentState.dstAlphaBlendFactor                 = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState.alphaBlendOp                        = VK_BLEND_OP_ADD;
        blendAttachmentState.colorWriteMask                      = renderStage->GetAttachment(attachmentBinding)->GetColorWriteMask();
        blendAttachmentStates.emplace_back(blendAttachmentState);
    }

    colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
    colorBlendState.pAttachments    = blendAttachmentStates.data();

    CreatePipeline();
}

void PipelineGraphics::CreatePipelineImgui()
{
    blendAttachmentStates[0].blendEnable         = VK_TRUE;
    blendAttachmentStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachmentStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentStates[0].colorBlendOp        = VK_BLEND_OP_ADD;
    blendAttachmentStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachmentStates[0].alphaBlendOp        = VK_BLEND_OP_ADD;
    blendAttachmentStates[0].colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
    colorBlendState.pAttachments    = blendAttachmentStates.data();

    CreatePipeline();
}

void PipelineGraphics::CreatePipelineStereo()
{
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 2;
    viewportState.scissorCount  = 2;

    CreatePipeline();
}

void PipelineGraphics::CreatePipelineStereoMRT()
{
    auto renderStage        = Graphics::Get()->GetRenderStage(stage.first);
    auto attachmentBindings = renderStage->GetOutputAttachmentBindings(stage.second);

    std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
    blendAttachmentStates.reserve(attachmentBindings.size());

    for (const auto& attachmentBinding : attachmentBindings) {
        VkPipelineColorBlendAttachmentState blendAttachmentState = {};
        blendAttachmentState.blendEnable                         = renderStage->GetAttachment(attachmentBinding)->EnableBlend() ? VK_TRUE : VK_FALSE;
        blendAttachmentState.srcColorBlendFactor                 = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachmentState.dstColorBlendFactor                 = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState.colorBlendOp                        = VK_BLEND_OP_ADD;
        blendAttachmentState.srcAlphaBlendFactor                 = VK_BLEND_FACTOR_ONE;
        blendAttachmentState.dstAlphaBlendFactor                 = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState.alphaBlendOp                        = VK_BLEND_OP_ADD;
        blendAttachmentState.colorWriteMask                      = renderStage->GetAttachment(attachmentBinding)->GetColorWriteMask();
        blendAttachmentStates.emplace_back(blendAttachmentState);
    }

    colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
    colorBlendState.pAttachments    = blendAttachmentStates.data();

    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 2;
    viewportState.scissorCount  = 2;

    CreatePipeline();
}
}   // namespace MapleLeaf