#pragma once

#include "CommandBuffer.hpp"
#include "NonCopyable.hpp"
#include "Pipeline.hpp"
#include "TypeInfo.hpp"


namespace MapleLeaf {
class Subrender : NonCopyable
{
public:
    explicit Subrender(Pipeline::Stage stage)
        : stage(std::move(stage))
    {}

    virtual ~Subrender() = default;

    virtual void PreRender(const CommandBuffer& commandBuffer) = 0;
    /**
     * Runs the render pipeline in the current renderpass.
     * @param commandBuffer The command buffer to record render command into.
     */
    virtual void Render(const CommandBuffer& commandBuffer) = 0;

    virtual void PostRender(const CommandBuffer& commandBuffer) = 0;

    virtual void RegisterImGui() = 0;

    const Pipeline::Stage& GetStage() const { return stage; }

    bool IsEnabled() const { return enabled; }
    void SetEnabled(bool enable) { this->enabled = enable; }

private:
    bool            enabled = true;
    Pipeline::Stage stage;
};

template class TypeInfo<Subrender>;
}   // namespace MapleLeaf