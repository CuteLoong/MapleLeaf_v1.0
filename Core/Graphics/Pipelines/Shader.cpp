#include "Shader.hpp"
#include "AccelerationStruct.hpp"
#include "Files.hpp"
#include "Graphics.hpp"
#include "Log.hpp"
#include "StorageBuffer.hpp"
#include "UniformBuffer.hpp"

#include "glslang/Public/ShaderLang.h"
#include "glslang/SPIRV/GlslangToSpv.h"

#include "config.h"
#include "spirv_reflect.h"

namespace MapleLeaf {
const std::vector<const char*> Shader::BindelssLayouts = {"ImageSamplers",
                                                          "MeshShadowMapping",
                                                          "MeshRadianceMapping",
                                                          "PrevMeshDepthMapping",
                                                          "MeshDepthMapping",
                                                          "PrevMeshRadianceMapping",
                                                          "MeshMaxDepthMapping",
                                                          "PrevMaxMeshDepthMapping"};

class ShaderIncluder : public glslang::TShader::Includer
{
public:
    IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth) override
    {
        auto directory  = std::filesystem::path(includerName).parent_path();
        auto fileLoaded = Files::Get()->Read(directory / headerName);

        if (!fileLoaded) {
            Log::Error("Shader Include could not be loaded: ", std::quoted(headerName), '\n');
            return nullptr;
        }

        auto content = new char[fileLoaded->size()];
        std::memcpy(content, fileLoaded->c_str(), fileLoaded->size());
        return new IncludeResult(headerName, content, fileLoaded->size(), content);
    }

    IncludeResult* includeSystem(const char* headerName, const char* includerName, size_t inclusionDepth) override
    {
        auto fileLoaded = Files::Get()->Read(headerName);

        if (!fileLoaded) {
            Log::Error("Shader Include could not be loaded: ", std::quoted(headerName), '\n');
            return nullptr;
        }

        auto content = new char[fileLoaded->size()];
        std::memcpy(content, fileLoaded->c_str(), fileLoaded->size());
        return new IncludeResult(headerName, content, fileLoaded->size(), content);
    }

    void releaseInclude(IncludeResult* result) override
    {
        if (result) {
            delete[] static_cast<char*>(result->userData);
            delete result;
        }
    }
};

Shader::Shader() {}

bool Shader::ReportedNotFound(const std::string& name, bool reportIfFound) const
{
    if (std::find(notFoundNames.begin(), notFoundNames.end(), name) == notFoundNames.end()) {
        if (reportIfFound) {
            notFoundNames.emplace_back(name);
        }

        return true;
    }
    return false;
}

// reflect will get a gltype
VkFormat Shader::GlTypeToVk(int32_t type)
{
    switch (type) {
    case 0x1406:   // GL_FLOAT
        return VK_FORMAT_R32_SFLOAT;
    case 0x8B50:   // GL_FLOAT_VEC2
        return VK_FORMAT_R32G32_SFLOAT;
    case 0x8B51:   // GL_FLOAT_VEC3
        return VK_FORMAT_R32G32B32_SFLOAT;
    case 0x8B52:   // GL_FLOAT_VEC4
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case 0x1404:   // GL_INT
        return VK_FORMAT_R32_SINT;
    case 0x8B53:   // GL_INT_VEC2
        return VK_FORMAT_R32G32_SINT;
    case 0x8B54:   // GL_INT_VEC3
        return VK_FORMAT_R32G32B32_SINT;
    case 0x8B55:   // GL_INT_VEC4
        return VK_FORMAT_R32G32B32A32_SINT;
    case 0x1405:   // GL_UNSIGNED_INT
        return VK_FORMAT_R32_SINT;
    case 0x8DC6:   // GL_UNSIGNED_INT_VEC2
        return VK_FORMAT_R32G32_SINT;
    case 0x8DC7:   // GL_UNSIGNED_INT_VEC3
        return VK_FORMAT_R32G32B32_SINT;
    case 0x8DC8:   // GL_UNSIGNED_INT_VEC4
        return VK_FORMAT_R32G32B32A32_SINT;
    default: return VK_FORMAT_UNDEFINED;
    }
}

std::pair<std::optional<uint32_t>, std::optional<uint32_t>> Shader::GetDescriptorLocation(const std::string& name) const
{
    for (const auto& [setIndex, descriptorLocationInSet] : descriptorLocations)
        if (auto it = descriptorLocationInSet.find(name); it != descriptorLocationInSet.end()) return std::make_pair(setIndex, it->second);

    return std::make_pair(std::nullopt, std::nullopt);
}

std::optional<uint32_t> Shader::GetDescriptorSize(const std::string& name) const
{
    for (const auto& [setIndex, descriptorSizeInSet] : descriptorSizes)
        if (auto it = descriptorSizeInSet.find(name); it != descriptorSizeInSet.end()) return it->second;

    return std::nullopt;
}

std::optional<Shader::Uniform> Shader::GetUniform(const std::string& name) const
{
    if (auto it = uniforms.find(name); it != uniforms.end()) return it->second;
    return std::nullopt;
}

std::optional<Shader::UniformBlock> Shader::GetUniformBlock(const std::string& name) const
{
    if (auto it = uniformBlocks.find(name); it != uniformBlocks.end()) return it->second;
    Log::Error("Uniform Block not found: ", name, '\n');
    return std::nullopt;
}

// std::optional<Shader::Attribute> Shader::GetAttribute(const std::string& name) const
// {
//     if (auto it = attributes.find(name); it != attributes.end()) return it->second;
//     return std::nullopt;
// }

std::vector<VkPushConstantRange> Shader::GetPushConstantRanges() const
{
    std::vector<VkPushConstantRange> pushConstantRanges;
    uint32_t                         currentOffset = 0;

    for (const auto& [uniformBlockName, uniformBlock] : uniformBlocks) {
        if (uniformBlock.GetType() != UniformBlock::Type::Push) continue;

        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags          = uniformBlock.GetStageFlags();
        pushConstantRange.offset              = currentOffset;
        pushConstantRange.size                = static_cast<uint32_t>(uniformBlock.GetSize());
        pushConstantRanges.emplace_back(pushConstantRange);
        currentOffset += pushConstantRange.size;
    }
    return pushConstantRanges;
}

std::optional<VkDescriptorType> Shader::GetDescriptorType(uint32_t setIndex, uint32_t location) const
{
    if (auto it = descriptorTypes.find(setIndex); it != descriptorTypes.end())
        if (auto it2 = it->second.find(location); it2 != it->second.end()) return it2->second;

    return std::nullopt;
}

VkShaderStageFlagBits Shader::GetShaderStage(const std::filesystem::path& filename)
{
    auto fileExt = filename.extension().string();
    std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::tolower);

    if (fileExt == ".comp") return VK_SHADER_STAGE_COMPUTE_BIT;
    if (fileExt == ".vert") return VK_SHADER_STAGE_VERTEX_BIT;
    if (fileExt == ".tesc") return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    if (fileExt == ".tese") return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    if (fileExt == ".geom") return VK_SHADER_STAGE_GEOMETRY_BIT;
    if (fileExt == ".frag") return VK_SHADER_STAGE_FRAGMENT_BIT;
    if (fileExt == ".rgen") return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    if (fileExt == ".rint") return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
    if (fileExt == ".rahit") return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    if (fileExt == ".rchit") return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    if (fileExt == ".rmiss") return VK_SHADER_STAGE_MISS_BIT_KHR;
    return VK_SHADER_STAGE_ALL;
}

Shader::Uniform::DescriptorType Shader::GetDescriptorType(const SpvReflectDescriptorType& descriptorType) const
{
    switch (descriptorType) {
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: return Uniform::DescriptorType::SAMPLER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return Uniform::DescriptorType::COMBINED_IMAGE_SAMPLER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return Uniform::DescriptorType::SAMPLED_IMAGE;
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: return Uniform::DescriptorType::STORAGE_IMAGE;
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return Uniform::DescriptorType::UNIFORM_TEXEL_BUFFER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return Uniform::DescriptorType::STORAGE_TEXEL_BUFFER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return Uniform::DescriptorType::UNIFORM_BUFFER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: return Uniform::DescriptorType::STORAGE_BUFFER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return Uniform::DescriptorType::UNIFORM_BUFFER_DYNAMIC;
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return Uniform::DescriptorType::STORAGE_BUFFER_DYNAMIC;
    case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return Uniform::DescriptorType::INPUT_ATTACHMENT;
    case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: return Uniform::DescriptorType::ACCELERATION_STRUCTURE_KHR;
    default: return Uniform::DescriptorType::UNDEFINED;
    }
}

Shader::Uniform::ResourceType Shader::GetResourceType(const SpvReflectResourceType& resourceType) const
{
    switch (resourceType) {
    case SPV_REFLECT_RESOURCE_FLAG_SAMPLER: return Uniform::ResourceType::SAMPLER;
    case SPV_REFLECT_RESOURCE_FLAG_CBV: return Uniform::ResourceType::CBV;
    case SPV_REFLECT_RESOURCE_FLAG_SRV: return Uniform::ResourceType::SRV;
    default: return Uniform::ResourceType::UNDEFINED;
    }
}

EShLanguage GetEshLanguage(VkShaderStageFlags stageFlag)
{
    switch (stageFlag) {
    case VK_SHADER_STAGE_COMPUTE_BIT: return EShLangCompute;
    case VK_SHADER_STAGE_VERTEX_BIT: return EShLangVertex;
    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return EShLangTessControl;
    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return EShLangTessEvaluation;
    case VK_SHADER_STAGE_GEOMETRY_BIT: return EShLangGeometry;
    case VK_SHADER_STAGE_FRAGMENT_BIT: return EShLangFragment;
    case VK_SHADER_STAGE_RAYGEN_BIT_KHR: return EShLangRayGen;
    case VK_SHADER_STAGE_INTERSECTION_BIT_KHR: return EShLangIntersect;
    case VK_SHADER_STAGE_ANY_HIT_BIT_KHR: return EShLangAnyHit;
    case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: return EShLangClosestHit;
    case VK_SHADER_STAGE_MISS_BIT_KHR: return EShLangMiss;
    default: return EShLangCount;
    }
}

TBuiltInResource GetResources()
{
    TBuiltInResource resources                            = {};
    resources.maxLights                                   = 32;
    resources.maxClipPlanes                               = 6;
    resources.maxTextureUnits                             = 32;
    resources.maxTextureCoords                            = 32;
    resources.maxVertexAttribs                            = 64;
    resources.maxVertexUniformComponents                  = 4096;
    resources.maxVaryingFloats                            = 64;
    resources.maxVertexTextureImageUnits                  = 32;
    resources.maxCombinedTextureImageUnits                = 80;
    resources.maxTextureImageUnits                        = 32;
    resources.maxFragmentUniformComponents                = 4096;
    resources.maxDrawBuffers                              = 32;
    resources.maxVertexUniformVectors                     = 128;
    resources.maxVaryingVectors                           = 8;
    resources.maxFragmentUniformVectors                   = 16;
    resources.maxVertexOutputVectors                      = 16;
    resources.maxFragmentInputVectors                     = 15;
    resources.minProgramTexelOffset                       = -8;
    resources.maxProgramTexelOffset                       = 7;
    resources.maxClipDistances                            = 8;
    resources.maxComputeWorkGroupCountX                   = 65535;
    resources.maxComputeWorkGroupCountY                   = 65535;
    resources.maxComputeWorkGroupCountZ                   = 65535;
    resources.maxComputeWorkGroupSizeX                    = 1024;
    resources.maxComputeWorkGroupSizeY                    = 1024;
    resources.maxComputeWorkGroupSizeZ                    = 64;
    resources.maxComputeUniformComponents                 = 1024;
    resources.maxComputeTextureImageUnits                 = 16;
    resources.maxComputeImageUniforms                     = 8;
    resources.maxComputeAtomicCounters                    = 8;
    resources.maxComputeAtomicCounterBuffers              = 1;
    resources.maxVaryingComponents                        = 60;
    resources.maxVertexOutputComponents                   = 64;
    resources.maxGeometryInputComponents                  = 64;
    resources.maxGeometryOutputComponents                 = 128;
    resources.maxFragmentInputComponents                  = 128;
    resources.maxImageUnits                               = 8;
    resources.maxCombinedImageUnitsAndFragmentOutputs     = 8;
    resources.maxCombinedShaderOutputResources            = 8;
    resources.maxImageSamples                             = 0;
    resources.maxVertexImageUniforms                      = 0;
    resources.maxTessControlImageUniforms                 = 0;
    resources.maxTessEvaluationImageUniforms              = 0;
    resources.maxGeometryImageUniforms                    = 0;
    resources.maxFragmentImageUniforms                    = 8;
    resources.maxCombinedImageUniforms                    = 8;
    resources.maxGeometryTextureImageUnits                = 16;
    resources.maxGeometryOutputVertices                   = 256;
    resources.maxGeometryTotalOutputComponents            = 1024;
    resources.maxGeometryUniformComponents                = 1024;
    resources.maxGeometryVaryingComponents                = 64;
    resources.maxTessControlInputComponents               = 128;
    resources.maxTessControlOutputComponents              = 128;
    resources.maxTessControlTextureImageUnits             = 16;
    resources.maxTessControlUniformComponents             = 1024;
    resources.maxTessControlTotalOutputComponents         = 4096;
    resources.maxTessEvaluationInputComponents            = 128;
    resources.maxTessEvaluationOutputComponents           = 128;
    resources.maxTessEvaluationTextureImageUnits          = 16;
    resources.maxTessEvaluationUniformComponents          = 1024;
    resources.maxTessPatchComponents                      = 120;
    resources.maxPatchVertices                            = 32;
    resources.maxTessGenLevel                             = 64;
    resources.maxViewports                                = 16;
    resources.maxVertexAtomicCounters                     = 0;
    resources.maxTessControlAtomicCounters                = 0;
    resources.maxTessEvaluationAtomicCounters             = 0;
    resources.maxGeometryAtomicCounters                   = 0;
    resources.maxFragmentAtomicCounters                   = 8;
    resources.maxCombinedAtomicCounters                   = 8;
    resources.maxAtomicCounterBindings                    = 1;
    resources.maxVertexAtomicCounterBuffers               = 0;
    resources.maxTessControlAtomicCounterBuffers          = 0;
    resources.maxTessEvaluationAtomicCounterBuffers       = 0;
    resources.maxGeometryAtomicCounterBuffers             = 0;
    resources.maxFragmentAtomicCounterBuffers             = 1;
    resources.maxCombinedAtomicCounterBuffers             = 1;
    resources.maxAtomicCounterBufferSize                  = 16384;
    resources.maxTransformFeedbackBuffers                 = 4;
    resources.maxTransformFeedbackInterleavedComponents   = 64;
    resources.maxCullDistances                            = 8;
    resources.maxCombinedClipAndCullDistances             = 8;
    resources.maxSamples                                  = 4;
    resources.limits.nonInductiveForLoops                 = true;
    resources.limits.whileLoops                           = true;
    resources.limits.doWhileLoops                         = true;
    resources.limits.generalUniformIndexing               = true;
    resources.limits.generalAttributeMatrixVectorIndexing = true;
    resources.limits.generalVaryingIndexing               = true;
    resources.limits.generalSamplerIndexing               = true;
    resources.limits.generalVariableIndexing              = true;
    resources.limits.generalConstantMatrixVectorIndexing  = true;
    return resources;
}

VkShaderModule Shader::CreateShaderModule(const std::filesystem::path& moduleName, const std::string& moduleCode, const std::string& preamble,
                                          VkShaderStageFlags moduleFlag)
{
    auto logicalDevice = Graphics::Get()->GetLogicalDevice();

    stages.emplace_back(moduleName);

    // Starts converting GLSL to SPIR-V.
    auto              language = GetEshLanguage(moduleFlag);
    glslang::TProgram program;
    glslang::TShader  shader(language);
    auto              resources = GetResources();

    auto messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules | EShMsgDefault);
#ifdef MAPLELEAF_SHADER_DEBUG
    messages = static_cast<EShMessages>(messages | EShMsgDebugInfo);
#endif

    auto      shaderName     = moduleName.string();
    auto      shaderNameCstr = shaderName.c_str();
    auto      shaderSource   = moduleCode.c_str();
    const int shaderLength   = static_cast<int>(moduleCode.size());

    shader.setStringsWithLengthsAndNames(&shaderSource, &shaderLength, &shaderNameCstr, 1);
    shader.setPreamble(preamble.c_str());

    auto defaultVersion = glslang::EShTargetVulkan_1_4;
    shader.setEnvInput(glslang::EShSourceGlsl, language, glslang::EShClientVulkan, 460);
    shader.setEnvClient(glslang::EShClientVulkan, defaultVersion);
    shader.setEnvTarget(glslang::EShTargetSpv,
                        volkGetInstanceVersion() >= VK_API_VERSION_1_3 ? glslang::EShTargetSpv_1_6 : glslang::EShTargetSpv_1_3);

    ShaderIncluder includer;

    std::string str;

    if (!shader.preprocess(&resources, defaultVersion, ENoProfile, false, false, messages, &str, includer)) {
        Log::Out(shader.getInfoLog(), '\n');
        Log::Out(shader.getInfoDebugLog(), '\n');
        Log::Error("SPRIV shader preprocess failed!\n");
    }

    if (!shader.parse(&resources, defaultVersion, true, messages, includer)) {
        Log::Out(shader.getInfoLog(), '\n');
        Log::Out(shader.getInfoDebugLog(), '\n');
        Log::Error("SPRIV shader parse failed!\n");
    }

    program.addShader(&shader);

    if (!program.link(messages) || !program.mapIO()) {
        Log::Error("Error while linking shader program.\n");
    }

    glslang::SpvOptions spvOptions;

#ifdef MAPLELEAF_SHADER_DEBUG
    spvOptions.generateDebugInfo = true;
    spvOptions.disableOptimizer  = true;
    spvOptions.optimizeSize      = false;
#else
    spvOptions.generateDebugInfo = false;
    spvOptions.disableOptimizer  = false;
    spvOptions.optimizeSize      = true;
#endif

    spv::SpvBuildLogger   logger;
    std::vector<uint32_t> spirv;
    GlslangToSpv(*program.getIntermediate(static_cast<EShLanguage>(language)), spirv, &logger, &spvOptions);

    SpvReflectShaderModule module;
    SpvReflectResult       result = spvReflectCreateShaderModule(spirv.size() * sizeof(uint32_t), spirv.data(), &module);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        Log::Error("Failed to create SPIR-V reflection module.\n");
        return VK_NULL_HANDLE;
    }

    if (module.entry_point_name == nullptr) {
        Log::Error("Failed to find entry point in shader module.\n");
        return VK_NULL_HANDLE;
    }
    else {
        const SpvReflectEntryPoint* entryPoint = spvReflectGetEntryPoint(&module, module.entry_point_name);
        if (entryPoint->spirv_execution_model == SpvExecutionModelGLCompute) {
            localSizes[0] = entryPoint->local_size.x;
            localSizes[1] = entryPoint->local_size.y;
            localSizes[2] = entryPoint->local_size.z;
        }

        LoadUniform(module, moduleFlag);
        LoadPushConstant(module, moduleFlag);
    }

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize                 = spirv.size() * sizeof(uint32_t);
    shaderModuleCreateInfo.pCode                    = spirv.data();

    VkShaderModule shaderModule;
    Graphics::CheckVk(vkCreateShaderModule(*logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule));
    return shaderModule;
}

void Shader::CreateReflection()
{
    std::map<VkDescriptorType, uint32_t> descriptorPoolCounts;
    auto                                 logicalDevice               = Graphics::Get()->GetLogicalDevice();
    uint32_t                             bindlessMaxDescriptorsCount = logicalDevice->GetBindlessMaxDescriptorsCount();

    // Process to descriptors.
    for (const auto& [uniformBlockName, uniformBlock] : uniformBlocks) {
        auto     descriptorType     = VK_DESCRIPTOR_TYPE_MAX_ENUM;
        int32_t  descriptorSetIndex = uniformBlock.set;
        uint32_t descriptorCount    = 1;

        if (uniformBlock.IsRunTimeArray()) {
            descriptorCount = bindlessMaxDescriptorsCount;
            if (descriptorSetInfos.count(descriptorSetIndex) != 0) Log::Error("This descriptor is runtime array, but have many bindings!");
            descriptorSetInfos.emplace(descriptorSetIndex, DescriptorSetInfo(true, 1));
        }
        else {
            if (descriptorSetInfos.count(descriptorSetIndex) == 0)
                descriptorSetInfos.emplace(descriptorSetIndex, DescriptorSetInfo(false, 1));
            else
                descriptorSetInfos[descriptorSetIndex].bindingCount++;
        }

        switch (uniformBlock.type) {
        case UniformBlock::Type::Uniform:
            descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorSetLayouts[descriptorSetIndex].emplace_back(UniformBuffer::GetDescriptorSetLayout(
                static_cast<uint32_t>(uniformBlock.binding), descriptorType, uniformBlock.stageFlags, descriptorCount));
            break;
        case UniformBlock::Type::Storage:
            descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorSetLayouts[descriptorSetIndex].emplace_back(StorageBuffer::GetDescriptorSetLayout(
                static_cast<uint32_t>(uniformBlock.binding), descriptorType, uniformBlock.stageFlags, descriptorCount));
            break;
        case UniformBlock::Type::Push: break;
        default: break;
        }

        IncrementDescriptorPool(descriptorPoolCounts, descriptorType);
        descriptorLocations[descriptorSetIndex].emplace(uniformBlockName, uniformBlock.binding);
        descriptorSizes[descriptorSetIndex].emplace(uniformBlockName, uniformBlock.size);
    }

    for (const auto& [uniformName, uniform] : uniforms) {
        auto     descriptorType     = VK_DESCRIPTOR_TYPE_MAX_ENUM;
        int32_t  descriptorSetIndex = uniform.set;
        uint32_t descriptorCount    = 1;

        if (uniform.IsRunTimeArray()) {
            descriptorCount = bindlessMaxDescriptorsCount;
            if (descriptorSetInfos.count(descriptorSetIndex) != 0) Log::Error("This descriptor is runtime array, but have many bindings!");
            descriptorSetInfos.emplace(descriptorSetIndex, DescriptorSetInfo(true, 1));
        }
        else {
            if (descriptorSetInfos.count(descriptorSetIndex) == 0)
                descriptorSetInfos.emplace(descriptorSetIndex, DescriptorSetInfo(false, 1));
            else
                descriptorSetInfos[descriptorSetIndex].bindingCount++;
        }

        if (uniform.descriptorType == Uniform::DescriptorType::ACCELERATION_STRUCTURE_KHR) {
            descriptorSetLayouts[descriptorSetIndex].emplace_back(
                AccelerationStruct::GetDescriptorSetLayout(static_cast<uint32_t>(uniform.binding), uniform.stageFlags, descriptorCount));
        }
        else {
            switch (uniform.descriptorType) {
            case Uniform::DescriptorType::INPUT_ATTACHMENT: descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; break;
            case Uniform::DescriptorType::COMBINED_IMAGE_SAMPLER: descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; break;
            case Uniform::DescriptorType::SAMPLED_IMAGE: descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; break;
            case Uniform::DescriptorType::STORAGE_IMAGE: descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; break;
            default: break;
            }
            descriptorSetLayouts[descriptorSetIndex].emplace_back(
                Image::GetDescriptorSetLayout(static_cast<uint32_t>(uniform.binding), descriptorType, uniform.stageFlags, descriptorCount));
        }

        IncrementDescriptorPool(descriptorPoolCounts, descriptorType);
        descriptorLocations[descriptorSetIndex].emplace(uniformName, uniform.binding);
        descriptorSizes[descriptorSetIndex].emplace(uniformName, uniform.size);
    }

    for (const auto& [type, descriptorCount] : descriptorPoolCounts) {
        VkDescriptorPoolSize descriptorPoolSize = {};
        descriptorPoolSize.type                 = type;
        descriptorPoolSize.descriptorCount      = bindlessMaxDescriptorsCount;
        descriptorPools.emplace_back(descriptorPoolSize);
    }

    // set a default descriptor pool, without any mean
    if (descriptorPoolCounts.empty()) {
        VkDescriptorPoolSize descriptorPoolSize = {};
        descriptorPoolSize.type                 = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorPoolSize.descriptorCount      = bindlessMaxDescriptorsCount;
        descriptorPools.emplace_back(descriptorPoolSize);
    }

    // descriptorPools.resize(6);
    // descriptorPools[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    // descriptorPools[0].descriptorCount = 4096;
    // descriptorPools[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // descriptorPools[1].descriptorCount = 2048;
    // descriptorPools[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    // descriptorPools[2].descriptorCount = 2048;
    // descriptorPools[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    // descriptorPools[3].descriptorCount = 2048;
    // descriptorPools[4].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    // descriptorPools[4].descriptorCount = 2048;
    // descriptorPools[5].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    // descriptorPools[5].descriptorCount = 2048;

    for (auto& [setIndex, descriptorSetLayoutInSet] : descriptorSetLayouts) {
        std::sort(descriptorSetLayoutInSet.begin(),
                  descriptorSetLayoutInSet.end(),
                  [](const VkDescriptorSetLayoutBinding& l, const VkDescriptorSetLayoutBinding& r) { return l.binding < r.binding; });

        if (!descriptorSetLayoutInSet.empty()) lastDescriptorBinding[setIndex] = descriptorSetLayoutInSet.back().binding;
        for (const auto& descriptor : descriptorSetLayoutInSet) descriptorTypes[setIndex].emplace(descriptor.binding, descriptor.descriptorType);
    }

    // Process attribute descriptions.
    // uint32_t currentOffset = 4;

    // for (const auto& [attributeName, attribute] : attributes) {
    //     VkVertexInputAttributeDescription attributeDescription = {};
    //     attributeDescription.location                          = static_cast<uint32_t>(attribute.location);
    //     attributeDescription.binding                           = 0;
    //     attributeDescription.format                            = GlTypeToVk(attribute.glType);
    //     attributeDescription.offset                            = currentOffset;
    //     attributeDescriptions.emplace_back(attributeDescription);
    //     currentOffset += attribute.size;
    // }
}

void Shader::IncrementDescriptorPool(std::map<VkDescriptorType, uint32_t>& descriptorPoolCounts, VkDescriptorType type)
{
    if (type == VK_DESCRIPTOR_TYPE_MAX_ENUM) return;

    if (auto it = descriptorPoolCounts.find(type); it != descriptorPoolCounts.end())
        it->second++;
    else
        descriptorPoolCounts.emplace(type, 1);
}

void Shader::LoadUniform(const SpvReflectShaderModule& module, VkShaderStageFlags stageFlag)
{
    uint32_t uniformCount = 0;
    spvReflectEnumerateDescriptorBindings(&module, &uniformCount, nullptr);
    std::vector<SpvReflectDescriptorBinding*> reflectUniforms(uniformCount);
    spvReflectEnumerateDescriptorBindings(&module, &uniformCount, reflectUniforms.data());

    for (const auto& uniform : reflectUniforms) {
        if (uniform->type_description->op == SpvOpTypeStruct) {
            const auto& block        = uniform->block;
            std::string name         = uniform->name;
            uint32_t    set          = uniform->set;
            uint32_t    binding      = uniform->binding;
            uint32_t    blockSize    = uniform->block.size;
            bool        runtimeArray = uniform->type_description->op == SpvOpTypeRuntimeArray;

            for (uint32_t i = 0; i < block.member_count; i++) {
                auto& member = block.members[i];
                blockSize += member.size;
            }

            for (auto& [uniformBlockName, uniformBlock] : uniformBlocks) {
                if (uniformBlockName == name) {
                    uniformBlock.stageFlags |= stageFlag;
                    continue;
                }
            }

            auto type = UniformBlock::Type::None;
            if (uniform->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                type = UniformBlock::Type::Uniform;
            else if (uniform->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
                type = UniformBlock::Type::AccStruct;
            else if (uniform->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                type = UniformBlock::Type::Storage;


            uniformBlocks.emplace(name, UniformBlock(uniform->set, uniform->binding, blockSize, stageFlag, type, runtimeArray));

            for (uint32_t i = 0; i < block.member_count; i++) {
                auto&       member     = block.members[i];
                std::string memberName = std::string(member.name);   //  name + "." +
                uniformBlocks[name].uniforms.emplace(memberName,
                                                     Uniform(set,
                                                             binding,
                                                             Uniform::DescriptorType::UNDEFINED,
                                                             Uniform::ResourceType::UNDEFINED,
                                                             member.offset,
                                                             member.size,
                                                             false,
                                                             stageFlag));
            }
        }
        else {
            std::string name           = uniform->name;
            uint32_t    set            = uniform->set;
            uint32_t    binding        = uniform->binding;
            uint32_t    blockSize      = uniform->block.size;
            auto        descriptorType = GetDescriptorType(uniform->descriptor_type);
            auto        resourceType   = GetResourceType(uniform->resource_type);
            bool        runtimeArray   = uniform->type_description->op == SpvOpTypeRuntimeArray;

            for (auto& [uniformName, uniform] : uniforms) {
                if (uniformName == name) {
                    uniform.stageFlags |= stageFlag;
                    continue;
                }
            }

            uniforms.emplace(name, Uniform(set, binding, descriptorType, resourceType, -1, -1, runtimeArray, stageFlag));
        }

        if (descriptorLocations.find(uniform->set) == descriptorLocations.end())
            descriptorLocations[uniform->set] = std::map<std::string, uint32_t>();
    }
}

void Shader::LoadPushConstant(const SpvReflectShaderModule& module, VkShaderStageFlags stageFlag)
{
    uint32_t constantCount = 0;
    spvReflectEnumeratePushConstantBlocks(&module, &constantCount, nullptr);
    std::vector<SpvReflectBlockVariable*> reflectConstants(constantCount);
    spvReflectEnumeratePushConstantBlocks(&module, &constantCount, reflectConstants.data());

    for (const auto& constant : reflectConstants) {
        std::string name = constant->name;
        uint32_t    size = constant->size;

        for (auto& [uniformBlockName, uniformBlock] : uniformBlocks) {
            if (uniformBlockName == name) {
                uniformBlock.stageFlags |= stageFlag;
                continue;
            }
        }

        uniformBlocks.emplace(name, UniformBlock(0, 0, size, stageFlag, UniformBlock::Type::Push));

        for (uint32_t i = 0; i < constant->member_count; i++) {
            auto&       member     = constant->members[i];
            std::string memberName = std::string(member.name);
            uniformBlocks[name].uniforms.emplace(
                memberName,
                Uniform(0, 0, Uniform::DescriptorType::UNDEFINED, Uniform::ResourceType::UNDEFINED, member.offset, member.size, false, stageFlag));
        }
    }
}

// void        LoadAttribute(const SpvReflectShaderModule& module, VkShaderStageFlags stageFlag)
// {
//     uint32_t attributeCount = 0;
//     spvReflectEnumerateInputVariables(&module, &attributeCount, nullptr);
//     std::vector<SpvReflectInterfaceVariable*> reflectAttributes(attributeCount);
//     spvReflectEnumerateInputVariables(&module, &attributeCount, reflectAttributes.data());

//     for (const auto& attribute : reflectAttributes) {
//         std::string name = attribute->name;
//         uint32_t    size = attribute->size;
//         uint32_t    loc  = attribute->location;

//         attributes.emplace(name, Attribute(loc, size, stageFlag));
//     }
// }
}   // namespace MapleLeaf
