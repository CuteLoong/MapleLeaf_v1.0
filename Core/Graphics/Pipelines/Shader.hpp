#pragma once

#include "volk.h"
#include <array>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "spirv_reflect.h"

namespace MapleLeaf {

class Shader
{
public:
    /**
     * A define added to the start of a shader, first value is the define name and second is the value to be set.
     */
    using Define = std::pair<std::string, std::string>;
    class VertexInput
    {
    public:
        VertexInput(std::vector<VkVertexInputBindingDescription>   bindingDescriptions   = {},
                    std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {})
            : bindingDescriptions(std::move(bindingDescriptions))
            , attributeDescriptions(std::move(attributeDescriptions))
        {}

        bool operator<(const VertexInput& rhs) const { return bindingDescriptions.front().binding < rhs.bindingDescriptions.front().binding; }

        const std::vector<VkVertexInputBindingDescription>&   GetBindingDescriptions() const { return bindingDescriptions; }
        const std::vector<VkVertexInputAttributeDescription>& GetAttributeDescriptions() const { return attributeDescriptions; }

    private:
        uint32_t                                       binding = 0;
        std::vector<VkVertexInputBindingDescription>   bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    };

    class Uniform
    {
        friend class Shader;

        enum class DescriptorType
        {
            UNDEFINED = 0,
            SAMPLER,
            COMBINED_IMAGE_SAMPLER,
            SAMPLED_IMAGE,
            STORAGE_IMAGE,
            UNIFORM_TEXEL_BUFFER,
            STORAGE_TEXEL_BUFFER,
            UNIFORM_BUFFER,
            STORAGE_BUFFER,
            UNIFORM_BUFFER_DYNAMIC,
            STORAGE_BUFFER_DYNAMIC,
            INPUT_ATTACHMENT,
            ACCELERATION_STRUCTURE_KHR
        };

        enum class ResourceType
        {
            UNDEFINED = 0,
            SAMPLER,
            CBV,
            SRV,
            UAV
        };

    public:
        explicit Uniform(uint32_t set, int32_t binding, DescriptorType descriptorType, ResourceType resourceType, int32_t offset = -1,
                         int32_t size = -1, bool runtimeArray = false, VkShaderStageFlags stageFlags = 0)
            : set(set)
            , binding(binding)
            , descriptorType(descriptorType)
            , resourceType(resourceType)
            , offset(offset)
            , size(size)
            , runtimeArray(runtimeArray)
            , stageFlags(stageFlags)
        {}

        uint32_t           GetSet() const { return set; }
        int32_t            GetBinding() const { return binding; }
        DescriptorType     GetDescriptorType() const { return descriptorType; }
        ResourceType       GetResourceType() const { return resourceType; }
        int32_t            GetOffset() const { return offset; }
        int32_t            GetSize() const { return size; }
        bool               IsRunTimeArray() const { return runtimeArray; }
        VkShaderStageFlags GetStageFlags() const { return stageFlags; }

        bool operator==(const Uniform& rhs) const
        {
            return set == rhs.set && binding == rhs.binding && descriptorType == rhs.descriptorType && resourceType == rhs.resourceType &&
                   offset == rhs.offset && size == rhs.size && stageFlags == rhs.stageFlags;
        }

        bool operator!=(const Uniform& rhs) const { return !operator==(rhs); }

    private:
        uint32_t           set;
        int32_t            binding;
        int32_t            offset;
        int32_t            size;
        DescriptorType     descriptorType;
        ResourceType       resourceType;
        bool               runtimeArray;
        VkShaderStageFlags stageFlags;
    };

    class UniformBlock
    {
        friend class Shader;

    public:
        enum class Type
        {
            None,
            Uniform,
            AccStruct,
            Storage,
            Push
        };
        explicit UniformBlock(uint32_t set = -1, int32_t binding = -1, int32_t size = -1, VkShaderStageFlags stageFlags = 0,
                              Type type = Type::Uniform, bool runtimeArray = false)
            : set(set)
            , binding(binding)
            , size(size)
            , stageFlags(stageFlags)
            , type(type)
            , runtimeArray(runtimeArray)
        {}

        uint32_t                              GetSet() const { return set; }
        int32_t                               GetBinding() const { return binding; }
        int32_t                               GetSize() const { return size; }
        VkShaderStageFlags                    GetStageFlags() const { return stageFlags; }
        Type                                  GetType() const { return type; }
        bool                                  IsRunTimeArray() const { return runtimeArray; }
        const std::map<std::string, Uniform>& GetUniforms() const { return uniforms; }

        std::optional<Uniform> GetUniform(const std::string& name) const
        {
            auto it = uniforms.find(name);

            if (it == uniforms.end()) {
                return std::nullopt;
            }

            return it->second;
        }

        bool operator==(const UniformBlock& rhs) const
        {
            return binding == rhs.binding && size == rhs.size && stageFlags == rhs.stageFlags && type == rhs.type;   // && uniforms == rhs.uniforms
        }

        bool operator!=(const UniformBlock& rhs) const { return !operator==(rhs); }

    private:
        uint32_t                       set;
        int32_t                        binding;
        int32_t                        size;
        VkShaderStageFlags             stageFlags;
        Type                           type;
        bool                           runtimeArray;
        std::map<std::string, Uniform> uniforms;
    };

    class Attribute
    {
        friend class Shader;

    public:
        explicit Attribute(int32_t set = -1, int32_t location = -1, int32_t size = -1, int32_t glType = -1)
            : set(set)
            , location(location)
            , size(size)
            , glType(glType)
        {}

        int32_t GetSet() const { return set; }
        int32_t GetLocation() const { return location; }
        int32_t GetSize() const { return size; }
        int32_t GetGlType() const { return glType; }

        bool operator==(const Attribute& rhs) const { return set == rhs.set && location == rhs.location && size == rhs.size && glType == rhs.glType; }

        bool operator!=(const Attribute& rhs) const { return !operator==(rhs); }

    private:
        int32_t set;
        int32_t location;
        int32_t size;
        int32_t glType;
    };

    // class Constant
    // {
    //     friend class Shader;

    // public:
    //     explicit Constant(int32_t binding = -1, int32_t size = -1, VkShaderStageFlags stageFlags = 0, int32_t glType = -1)
    //         : binding(binding)
    //         , size(size)
    //         , stageFlags(stageFlags)
    //         , glType(glType)
    //     {}

    //     int32_t            GetBinding() const { return binding; }
    //     int32_t            GetSize() const { return size; }
    //     VkShaderStageFlags GetStageFlags() const { return stageFlags; }
    //     int32_t            GetGlType() const { return glType; }

    //     bool operator==(const Constant& rhs) const
    //     {
    //         return binding == rhs.binding && size == rhs.size && stageFlags == rhs.stageFlags && glType == rhs.glType;
    //     }

    //     bool operator!=(const Constant& rhs) const { return !operator==(rhs); }

    // private:
    //     int32_t            binding;
    //     int32_t            size;
    //     VkShaderStageFlags stageFlags;
    //     int32_t            glType;
    // };

    struct DescriptorSetInfo
    {
        bool     IsRunTimeArray;
        uint32_t bindingCount;

        DescriptorSetInfo(bool IsRunTimeArray = false, uint32_t bindingCount = 0)
            : IsRunTimeArray(IsRunTimeArray)
            , bindingCount(bindingCount)
        {}
    };

    Shader();

    bool                                                        ReportedNotFound(const std::string& name, bool reportIfFound) const;
    static VkFormat                                             GlTypeToVk(int32_t type);
    std::pair<std::optional<uint32_t>, std::optional<uint32_t>> GetDescriptorLocation(const std::string& name) const;
    std::optional<uint32_t>                                     GetDescriptorSize(const std::string& name) const;
    std::optional<Uniform>                                      GetUniform(const std::string& name) const;
    std::optional<UniformBlock>                                 GetUniformBlock(const std::string& name) const;
    std::optional<Attribute>                                    GetAttribute(const std::string& name) const;
    std::vector<VkPushConstantRange>                            GetPushConstantRanges() const;

    std::optional<VkDescriptorType> GetDescriptorType(uint32_t setIndex, uint32_t location) const;
    static VkShaderStageFlagBits    GetShaderStage(const std::filesystem::path& filename);
    VkShaderModule CreateShaderModule(const std::filesystem::path& moduleName, const std::string& moduleCode, const std::string& preamble,
                                      VkShaderStageFlags moduleFlag);
    void           CreateReflection();

    Uniform::DescriptorType GetDescriptorType(const SpvReflectDescriptorType& descriptorType) const;
    Uniform::ResourceType   GetResourceType(const SpvReflectResourceType& resourceType) const;

    const std::filesystem::path&               GetName() const { return stages.back(); }
    const std::map<std::string, Uniform>&      GetUniforms() const { return uniforms; };
    const std::map<std::string, UniformBlock>& GetUniformBlocks() const { return uniformBlocks; };
    // const std::map<std::string, Attribute>&                              GetAttributes() const { return attributes; };
    const std::array<std::optional<uint32_t>, 3>&                        GetLocalSizes() const { return localSizes; }
    const std::vector<VkDescriptorPoolSize>&                             GetDescriptorPools() const { return descriptorPools; }
    const std::map<uint32_t, uint32_t>&                                  GetLastDescriptorBinding() const { return lastDescriptorBinding; }
    const std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>>& GetDescriptorSetLayouts() const { return descriptorSetLayouts; }
    const std::map<uint32_t, DescriptorSetInfo>&                         GetDescriptorSetInfos() const { return descriptorSetInfos; }

    // const std::vector<VkVertexInputAttributeDescription>&               GetAttributeDescriptions() const { return attributeDescriptions; }
    // const std::map<std::string, Constant>&                GetConstants() const { return constants; };

    static const std::vector<const char*> BindelssLayouts;

private:
    std::vector<std::filesystem::path>  stages;
    std::map<std::string, Uniform>      uniforms;
    std::map<std::string, UniformBlock> uniformBlocks;
    // std::map<std::string, Attribute>    attributes;
    // std::map<std::string, Constant>     constants;

    std::array<std::optional<uint32_t>, 3> localSizes;

    std::vector<VkDescriptorPoolSize> descriptorPools;

    // Vector's index is setIndex, value is DescriptorInfo about this descriptor set.
    std::map<uint32_t, std::map<std::string, uint32_t>>           descriptorLocations;
    std::map<uint32_t, std::map<std::string, uint32_t>>           descriptorSizes;
    std::map<uint32_t, std::map<uint32_t, VkDescriptorType>>      descriptorTypes;
    std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> descriptorSetLayouts;
    std::map<uint32_t, uint32_t>                                  lastDescriptorBinding;
    std::map<uint32_t, DescriptorSetInfo>                         descriptorSetInfos;
    // std::vector<VkVertexInputAttributeDescription>         attributeDescriptions;

    mutable std::vector<std::string> notFoundNames;

    static void IncrementDescriptorPool(std::map<VkDescriptorType, uint32_t>& descriptorPoolCounts, VkDescriptorType type);
    void        LoadUniform(const SpvReflectShaderModule& module, VkShaderStageFlags stageFlag);
    void        LoadPushConstant(const SpvReflectShaderModule& module, VkShaderStageFlags stageFlag);
    // void        LoadAttribute(const SpvReflectShaderModule& module, VkShaderStageFlags stageFlag);
};
}   // namespace MapleLeaf