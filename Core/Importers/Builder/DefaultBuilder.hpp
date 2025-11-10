#pragma once

#include "Animation.hpp"
#include "Camera.hpp"
#include "Image2d.hpp"
#include "Light.hpp"
#include "Mesh.hpp"
#include "SceneGraph.hpp"
#include <memory>
#include <mikktspace.h>
#include <unordered_map>


namespace MapleLeaf {
class MikkTSpaceWrapper
{
public:
    static bool GenerateTangents(std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices)
    {
        SMikkTSpaceInterface mikkTInterface = {};
        mikkTInterface.m_getNumFaces        = [](const SMikkTSpaceContext* pContext) {
            return ((MikkTSpaceWrapper*)(pContext->m_pUserData))->getFaceCount();
        };
        mikkTInterface.m_getNumVerticesOfFace = [](const SMikkTSpaceContext* pContext, const int32_t face) { return 3; };
        mikkTInterface.m_getPosition          = [](const SMikkTSpaceContext* pContext, float position[], const int32_t face, const int32_t vert) {
            ((MikkTSpaceWrapper*)(pContext->m_pUserData))->getPosition(position, face, vert);
        };
        mikkTInterface.m_getNormal = [](const SMikkTSpaceContext* pContext, float normal[], const int32_t face, const int32_t vert) {
            ((MikkTSpaceWrapper*)(pContext->m_pUserData))->getNormal(normal, face, vert);
        };
        mikkTInterface.m_getTexCoord = [](const SMikkTSpaceContext* pContext, float texCrd[], const int32_t face, const int32_t vert) {
            ((MikkTSpaceWrapper*)(pContext->m_pUserData))->getTexCrd(texCrd, face, vert);
        };
        mikkTInterface.m_setTSpaceBasic =
            [](const SMikkTSpaceContext* pContext, const float tangent[], const float sign, const int32_t face, const int32_t vert) {
                ((MikkTSpaceWrapper*)(pContext->m_pUserData))->setTangent(tangent, sign, face, vert);
            };

        MikkTSpaceWrapper  wrapper(vertices, indices);
        SMikkTSpaceContext context = {};
        context.m_pUserData        = &wrapper;
        context.m_pInterface       = &mikkTInterface;

        if (!genTangSpaceDefault(&context)) {
            Log::Error("MikkTSpace: Failed to generate tangents.");
            return false;
        }
        return true;
    }

private:
    MikkTSpaceWrapper(std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices)
        : vertices(vertices)
        , indices(indices)
    {}

    std::vector<Vertex3D>&       vertices;
    const std::vector<uint32_t>& indices;

    int32_t getFaceCount() const { return static_cast<int32_t>(indices.size() / 3); }
    void    getPosition(float position[], int32_t face, int32_t vert) const
    {
        uint32_t    index = indices[face * 3 + vert];
        const auto& pos   = vertices[index].position;
        position[0]       = pos.x;
        position[1]       = pos.y;
        position[2]       = pos.z;
    }
    void getNormal(float normal[], int32_t face, int32_t vert) const
    {
        uint32_t    index = indices[face * 3 + vert];
        const auto& nrm   = vertices[index].normal;
        normal[0]         = nrm.x;
        normal[1]         = nrm.y;
        normal[2]         = nrm.z;
    }

    void getTexCrd(float texCrd[], int32_t face, int32_t vert) const
    {
        uint32_t    index = indices[face * 3 + vert];
        const auto& uv    = vertices[index].uv;
        texCrd[0]         = uv.x;
        texCrd[1]         = uv.y;
    }

    void setTangent(const float tangent[], float sign, int32_t face, int32_t vert)
    {
        uint32_t  index = indices[face * 3 + vert];
        glm::vec3 T(tangent[0], tangent[1], tangent[2]);
        vertices[index].tangent = glm::vec4(normalize(T), sign);
    }
};


class Builder
{
    friend class SceneBuilder;

public:
    Builder() = default;

    Mesh* GetMesh(const uint32_t index);

    NodeID AddSceneNode(SceneNode&& node);
    void   AddLight(std::unique_ptr<Light>&& light);
    void   AddCamera(std::unique_ptr<Camera>&& camera);
    void   AddAnimation(NodeID nodeID, std::shared_ptr<Animation>& animation);

    template<typename T, typename = std::enable_if_t<std::is_convertible_v<T*, Material*>>>
    Mesh* AddMesh(std::shared_ptr<Model>&& model, std::shared_ptr<T> material)
    {
        meshes.push_back(std::make_unique<Mesh>(model, material));
        return meshes.back().get();
    }

    template<typename T, typename = std::enable_if_t<std::is_convertible_v<T*, Material*>>>
    Mesh* AddMesh(std::shared_ptr<Vertex3D>&& vertexBuffer, std::vector<uint32_t>&& indexBuffer, std::shared_ptr<T> material)
    {}

    template<typename T, typename = std::enable_if_t<std::is_convertible_v<T*, Material*>>>
    bool loadMaterialTexture(std::shared_ptr<T>& material, Material::TextureSlot textureType, const std::filesystem::path& path)
    {
        if (material == nullptr) return false;

        auto image = Image2d::Create(path);

        if (textureType == Material::TextureSlot::BaseColor)
            material->SetImageDiffuse(std::move(image));
        else if (textureType == Material::TextureSlot::Emissive)
            material->SetImageEmissive(std::move(image));
        else if (textureType == Material::TextureSlot::Normal)
            material->SetImageNormal(std::move(image));
        else if (textureType == Material::TextureSlot::Material)
            material->SetImageMaterial(std::move(image));
        else {
            Log::Error("Material::TextureSlot is not exit!");
            return false;
        }

        return true;
    }

private:
    std::vector<std::unique_ptr<Mesh>>                                         meshes;
    std::vector<std::unique_ptr<Light>>                                        lights;
    std::vector<std::unique_ptr<Camera>>                                       cameras;
    std::unordered_map<NodeID, std::shared_ptr<Animation>, NodeID::NodeIDHash> animations;
    SceneGraph                                                                 sceneGraph;
};
}   // namespace MapleLeaf