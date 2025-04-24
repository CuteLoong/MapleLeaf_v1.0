#include "Skybox.hpp"

namespace MapleLeaf {
Skybox::Skybox(const std::string& filename, bool enableRotation)
    : filename(filename)
{
    // 判断文件后缀名
    // if (filename.find(".png") == std::string::npos && filename.find(".jpg") == std::string::npos) {
    //     filename += ".png";
    // }
    if (filename.find(".png") != std::string::npos || filename.find(".") == std::string::npos) {
        skyboxCube = ImageCube::Create(filename, ".png");
    }
    else if (filename.find(".exr") != std::string::npos) {
        skyboxImage = Image2d::Create(filename, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, false, false);
        skyboxCube  = std::make_shared<ImageCube>(glm::uvec2(1024, 1024),
                                                 skyboxImage->GetFormat(),
                                                 VK_IMAGE_LAYOUT_GENERAL,
                                                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                                                 VK_FILTER_LINEAR,
                                                 VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                 VK_SAMPLE_COUNT_1_BIT,
                                                 false,
                                                 true);
    }

    if (!skyboxCube) {
        Log::Error("Could not load skybox ", filename, '\n');
    }

    transform = std::make_unique<Transform>();
}

void Skybox::SetRotation(const glm::vec3& rotation)
{
    glm::quat quat = glm::angleAxis(glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)) *
                     glm::angleAxis(glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
                     glm::angleAxis(glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    if (transform)
        transform->SetLocalRotation(quat);
    else
        transform = std::make_unique<Transform>(glm::vec3(0.0f), quat, glm::vec3(1.0f));
}
}   // namespace MapleLeaf