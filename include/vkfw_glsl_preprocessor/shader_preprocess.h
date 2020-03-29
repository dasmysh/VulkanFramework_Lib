/**
 * @file   shader_preprocess.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.03.29
 *
 * @brief  Definition of the recursive shader loading.
 */

#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace vkfw_glsl {

    class shader_processor
    {
    public:
        explicit shader_processor(std::vector<std::filesystem::path> t_file_paths, std::vector<std::string> t_defines = {});

        [[nodiscard]] std::string process_shader(const std::filesystem::path& shader_file);
    private:
        [[nodiscard]] std::filesystem::path find_file_location(const std::filesystem::path& relative_path);
        [[nodiscard]] std::string process_shader_recursive(const std::filesystem::path& shader_file,
                                                           std::size_t& file_id,
                                             std::size_t recursion_depth);

        std::vector<std::filesystem::path> m_file_paths;
        std::vector<std::string> m_defines;
    };

}
