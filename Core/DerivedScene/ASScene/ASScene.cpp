#include "ASScene.hpp"

#include "BottomLevelAccelerationStruct.hpp"
#include "Mesh.hpp"
#include "Model.hpp"
#include "Resources.hpp"
#include "Scenes.hpp"
#include "TopLevelAccelerationStruct.hpp"

namespace {
using namespace MapleLeaf;

struct TempNode : ASScene::BVHNode
{
    uint32_t visitOrder = InvalidMask;
    uint32_t parent     = InvalidMask;

    uint32_t left;
    uint32_t right;

    glm::vec3 aabbCenter;
};

ASScene::AABB CalculateBounds(glm::mat4 world, glm::vec3 min, glm::vec3 max)
{
    glm::vec3 minWorld = glm::vec3(FLT_MAX);
    glm::vec3 maxWorld = glm::vec3(-FLT_MAX);

    for (int i = 0; i < 8; i++) {
        glm::vec3 p = glm::vec3((i & 1) ? max.x : min.x, (i & 2) ? max.y : min.y, (i & 4) ? max.z : min.z);
        p           = glm::vec3(world * glm::vec4(p, 1.0f));

        minWorld = glm::min(minWorld, p);
        maxWorld = glm::max(maxWorld, p);
    }

    return {minWorld, maxWorld};
}

ASScene::AABB CalculateBounds(const std::vector<TempNode>& nodes, uint32_t start, uint32_t end)
{
    ASScene::AABB bounds = {glm::vec3(std::numeric_limits<float>::max()), glm::vec3(std::numeric_limits<float>::lowest())};

    for (uint32_t i = start; i < end; ++i) {
        bounds.first  = glm::min(bounds.first, nodes[i].min);
        bounds.second = glm::max(bounds.second, nodes[i].max);
    }

    return bounds;
}

uint32_t Split(std::vector<TempNode>& nodes, uint32_t start, uint32_t end, ASScene::AABB& bounds)
{
    glm::vec3            extent      = bounds.second - bounds.first;
    std::array<float, 3> extentArray = {extent.x, extent.y, extent.z};
    uint32_t splitAxis = static_cast<uint32_t>(std::distance(extentArray.begin(), std::max_element(extentArray.begin(), extentArray.end())));

    std::sort(nodes.begin() + start, nodes.begin() + end, [splitAxis](const TempNode& a, const TempNode& b) {
        return a.aabbCenter[splitAxis] < b.aabbCenter[splitAxis];
    });

    float splitPos = nodes[start + (end - start) / 2].aabbCenter[splitAxis];
    for (uint32_t i = start + 1; i < end; ++i) {
        if (nodes[i].aabbCenter[splitAxis] >= splitPos) {
            return i;
        }
    }

    return end - 1;
}

uint32_t RecursiveBuildBVH(std::vector<TempNode>& nodes, uint32_t start, uint32_t end)
{
    uint32_t count = end - start;
    if (count == 1) return start;

    ASScene::AABB bounds = CalculateBounds(nodes, start, end);

    uint32_t split = Split(nodes, start, end, bounds);

    uint32_t nodeId = nodes.size();
    nodes.emplace_back(TempNode{});

    nodes[nodeId].left  = RecursiveBuildBVH(nodes, start, split);
    nodes[nodeId].right = RecursiveBuildBVH(nodes, split, end);

    nodes[nodeId].SetBounds(bounds);
    nodes[nodeId].aabbCenter = (bounds.first + bounds.second) * 0.5f;
    nodes[nodeId].instanceID = ASScene::BVHNode::InvalidMask;

    nodes[nodes[nodeId].left].parent  = nodeId;
    nodes[nodes[nodeId].right].parent = nodeId;

    return nodeId;
}

void SetDepthFirstVisitOrder(std::vector<TempNode>& nodes, uint32_t nodeID, uint32_t nextID, uint32_t& visitOrder)
{
    TempNode& node = nodes[nodeID];

    node.visitOrder = visitOrder++;
    node.next       = nextID;

    if (node.left != ASScene::BVHNode::InvalidMask) SetDepthFirstVisitOrder(nodes, node.left, node.right, visitOrder);
    if (node.right != ASScene::BVHNode::InvalidMask) SetDepthFirstVisitOrder(nodes, node.right, nextID, visitOrder);
}

void SetDepthFirstVisitOrder(std::vector<TempNode>& nodes, uint32_t root)
{
    uint32_t visitOrder = 0;
    SetDepthFirstVisitOrder(nodes, root, ASScene::BVHNode::InvalidMask, visitOrder);
}
}   // namespace

namespace MapleLeaf {
ASScene::ASScene() {}

ASScene::~ASScene() {}

inline VkTransformMatrixKHR ASScene::ToTransformMatrixKHR(glm::mat4 matrix)
{
    glm::mat4            temp = glm::transpose(matrix);
    VkTransformMatrixKHR transformMatrixKHR{};
    memcpy(&transformMatrixKHR, &temp, sizeof(transformMatrixKHR));
    return transformMatrixKHR;
}

void ASScene::Start()
{
    // BuildBVH();
#ifdef MAPLELEAF_RAY_TRACING
    BuildBLAS();
    BuildTLAS();
#endif
}

void ASScene::Update()
{
    // BuildBVH();
#ifdef MAPLELEAF_RAY_TRACING
    // BuildBLAS(VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR, true);
    BuildTLAS(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR, true);
#endif
}

void ASScene::BuildBVH(bool update)
{
    bvhNodes.clear();
    bvhPackedNodes.clear();

    const auto& meshes = Scenes::Get()->GetScene()->GetComponents<Mesh>();

    std::vector<TempNode> nodes;
    nodes.reserve(2 * meshes.size() - 1);
    bvhNodes.reserve(2 * meshes.size() - 1);

    for (uint32_t i = 0; i < meshes.size(); ++i) {
        TempNode node;
        node.SetBounds(CalculateBounds(meshes[i]->GetEntity()->GetComponent<Transform>()->GetWorldMatrix(),
                                       meshes[i]->GetModel()->GetMinExtents(),
                                       meshes[i]->GetModel()->GetMaxExtents()));

        node.aabbCenter = (node.min + node.max) * 0.5f;
        node.instanceID = meshes[i]->GetInstanceId();
        node.left       = BVHNode::InvalidMask;
        node.right      = BVHNode::InvalidMask;

        nodes.emplace_back(node);
    }

    const uint32_t root = RecursiveBuildBVH(nodes, 0, static_cast<uint32_t>(nodes.size()));

    SetDepthFirstVisitOrder(nodes, root);

    bvhNodes.resize(nodes.size());
    bvhPackedNodes.reserve(bvhNodes.size() * 2);

    for (uint32_t i = 0; i < nodes.size(); i++) {
        const TempNode& oldNode = nodes[i];

        BVHNode& newNode   = bvhNodes[oldNode.visitOrder];
        newNode.min        = oldNode.min;
        newNode.max        = oldNode.max;
        newNode.instanceID = oldNode.instanceID;
        newNode.next       = oldNode.next == BVHNode::InvalidMask ? BVHNode::InvalidMask : nodes[oldNode.next].visitOrder;
    }

    bvhBuffer = std::make_unique<StorageBuffer>(bvhNodes.size() * sizeof(BVHNode), bvhNodes.data());
}

void ASScene::BuildBLAS(VkBuildAccelerationStructureFlagsKHR flags, bool update)
{
    const auto&  models      = Resources::Get()->FindAll<Model>();
    VkDeviceSize ASTotalSize = 0;
    VkDeviceSize scratchSize = 0;

    std::vector<ASBuildInfo> ASBuildInfos(models.size());

    for (size_t i = 0; i < models.size(); ++i) {
        const auto& model = models[i];

        VkAccelerationStructureBuildGeometryInfoKHR geometryInfo{};
        geometryInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        geometryInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        geometryInfo.mode          = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        geometryInfo.flags         = model->GetBLASInput()->flags | flags;
        geometryInfo.geometryCount = 1;
        geometryInfo.pGeometries   = &model->GetBLASInput()->geometry;

        VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
        buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        ASBuildInfos[i] = std::move(ASBuildInfo{geometryInfo, buildSizesInfo, model->GetBLASInput()->buildRangeInfo});

        AccelerationStruct::GetAccelerationStructureBuildSizes(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                                               &ASBuildInfos[i].buildGeometryInfo,
                                                               &ASBuildInfos[i].buildRangeInfo.primitiveCount,
                                                               &ASBuildInfos[i].buildSizesInfo);

        ASTotalSize += ASBuildInfos[i].buildSizesInfo.accelerationStructureSize;
        scratchSize = std::max(ASBuildInfos[i].buildSizesInfo.buildScratchSize, scratchSize);
    }

    Buffer scratchBuffer(
        scratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    bottomLevelaccelerationStructs.reserve(models.size());

    for (size_t i = 0; i < models.size(); ++i) {
        bottomLevelaccelerationStructs.emplace_back(
            std::make_unique<BottomLevelAccelerationStruct>(ASBuildInfos[i], scratchBuffer.GetDeviceAddress()));
    }
}

void ASScene::BuildTLAS(VkBuildAccelerationStructureFlagsKHR flags, bool update)
{
    const auto& meshes = Scenes::Get()->GetScene()->GetComponents<Mesh>();

    std::vector<VkAccelerationStructureInstanceKHR> instances{};
    instances.reserve(meshes.size());

    for (uint32_t i = 0; i < meshes.size(); ++i) {
        const auto& instance   = meshes[i];
        uint32_t    modelIndex = Resources::Get()->GetResourceIndex(instance->GetModel());

        VkAccelerationStructureInstanceKHR accelerationStructureInstance{};
        accelerationStructureInstance.transform           = ToTransformMatrixKHR(instance->GetEntity()->GetComponent<Transform>()->GetWorldMatrix());
        accelerationStructureInstance.instanceCustomIndex = instance->GetInstanceId();
        accelerationStructureInstance.mask                = 0xFF;
        accelerationStructureInstance.instanceShaderBindingTableRecordOffset = 0;
        accelerationStructureInstance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        accelerationStructureInstance.accelerationStructureReference         = bottomLevelaccelerationStructs[modelIndex]->GetDeviceAddress();

        instances.emplace_back(accelerationStructureInstance);
    }

    Buffer instancesBuffer(sizeof(VkAccelerationStructureInstanceKHR) * instances.size(),
                           VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           instances.data());

    VkAccelerationStructureGeometryInstancesDataKHR instancesData{};
    instancesData.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    instancesData.data.deviceAddress = instancesBuffer.GetDeviceAddress();

    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances = instancesData;

    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
    buildGeometryInfo.sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildGeometryInfo.type                     = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildGeometryInfo.mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildGeometryInfo.flags                    = flags;
    buildGeometryInfo.geometryCount            = 1;
    buildGeometryInfo.pGeometries              = &geometry;
    buildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;

    VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
    buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
    buildRangeInfo.primitiveCount  = static_cast<uint32_t>(instances.size());
    buildRangeInfo.primitiveOffset = 0;
    buildRangeInfo.firstVertex     = 0;
    buildRangeInfo.transformOffset = 0;

    ASBuildInfo buildInfo{buildGeometryInfo, buildSizesInfo, buildRangeInfo};

    AccelerationStruct::GetAccelerationStructureBuildSizes(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                                           &buildInfo.buildGeometryInfo,
                                                           &buildInfo.buildRangeInfo.primitiveCount,
                                                           &buildInfo.buildSizesInfo);

    topLevelAccelerationStruct = std::make_unique<TopLevelAccelerationStruct>(buildInfo);
}
}   // namespace MapleLeaf