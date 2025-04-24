#include "DefaultSubrender.hpp"
#include "PipelineGraphics.hpp"

namespace MapleLeaf {

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
};

DefaultSubrender::DefaultSubrender(const Pipeline::Stage& pipelineStage)
    : Subrender(pipelineStage)
    , pipeline(pipelineStage, {"Default/Default.vert", "Default/Default.frag"}, {}, {}, PipelineGraphics::Mode::Polygon,
               PipelineGraphics::Depth::None, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
               VK_FRONT_FACE_COUNTER_CLOCKWISE, false)
{}

void DefaultSubrender::RegisterImGui() {}

void DefaultSubrender::PreRender(const CommandBuffer& commandBuffer) {}

void DefaultSubrender::Render(const CommandBuffer& commandBuffer)
{
    pipeline.BindPipeline(commandBuffer);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void DefaultSubrender::PostRender(const CommandBuffer& commandBuffer) {}

}   // namespace MapleLeaf