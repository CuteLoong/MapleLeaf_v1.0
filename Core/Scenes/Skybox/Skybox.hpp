#pragma once

#include "Image2d.hpp"
#include "ImageCube.hpp"
#include "Transform.hpp"

namespace MapleLeaf {
class Skybox
{

public:
    explicit Skybox(const std::string& filename = "SkyboxClouds", bool enableRotation = false);

    const std::string& GetFilename() const { return filename; }

    const ImageCube* GetImageCube() const { return skyboxCube.get(); }
    const Image2d*   GetImage2d() const { return skyboxImage.get(); }
    const Transform* GetTransform() const { return transform.get(); }

    void SetRotation(const glm::vec3& rotation);

    bool GetMapped() const { return mapped; }
    void SetMapped(bool mapped) { this->mapped = mapped; }

private:
    std::string filename;
    bool        mapped = false;

    std::shared_ptr<ImageCube> skyboxCube;
    std::shared_ptr<Image2d>   skyboxImage;
    std::unique_ptr<Transform> transform;
};
}   // namespace MapleLeaf