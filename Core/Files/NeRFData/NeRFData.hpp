#pragma once

#include <fstream>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <vector>


namespace nlohmann {
template<>
struct adl_serializer<glm::mat4>
{
    static void to_json(nlohmann::json& j, const glm::mat4& mat)
    {
        j = nlohmann::json::array();
        for (int row = 0; row < 4; ++row) {
            nlohmann::json row_json = nlohmann::json::array();
            for (int col = 0; col < 4; ++col) {
                row_json.push_back(mat[col][row]);   // GLM is column-major order, converted to row-major order
            }
            j.push_back(row_json);
        }
    }

    static void from_json(const nlohmann::json& j, glm::mat4& mat)
    {
        for (int row = 0; row < 4; ++row) {
            const auto& row_json = j.at(row);
            for (int col = 0; col < 4; ++col) {
                mat[col][row] = row_json.at(col).get<float>();   // Convert row-major to column-major
            }
        }
    }
};
}   // namespace nlohmann

namespace MapleLeaf {
struct Frame
{
    std::string file_path;
    glm::mat4   transform_matrix = glm::mat4(1.0f);
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Frame, file_path, transform_matrix);

struct NeRFData
{
    float              camera_angle_x = 0.0f;
    std::vector<Frame> frames;

    void save(const std::string& path) const
    {
        nlohmann::json j;
        j["camera_angle_x"] = camera_angle_x;
        j["frames"]         = frames;

        std::ofstream file(path);
        if (!file) throw std::runtime_error("Failed to create file");
        file << j.dump(4);   // 带缩进的格式化输出
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NeRFData, camera_angle_x, frames);

}   // namespace MapleLeaf