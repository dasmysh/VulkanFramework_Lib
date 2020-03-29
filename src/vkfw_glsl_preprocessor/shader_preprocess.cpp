/**
 * @file   shader_preprocess.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.03.29
 *
 * @brief  Implementation of the recursive shader loading.
 */

#include "shader_preprocess.h"
#include "constants.h"

#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <algorithm>

namespace vkfw_glsl {

    // see https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
    static inline void ltrim(std::string& s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return std::isspace(ch) == 0; }));
    }

    // trim from end (in place)
    static inline void rtrim(std::string& s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return std::isspace(ch) == 0; }).base(), s.end());
    }

    // trim from both ends (in place)
    static inline void trim(std::string& s)
    {
        ltrim(s);
        rtrim(s);
    }

    // trim from both ends (copying)
    static inline std::string trim_copy(std::string s)
    {
        trim(s);
        return s;
    }

    shader_processor::shader_processor(std::vector<std::filesystem::path> t_file_paths,
                                       std::vector<std::string> t_defines)
        : m_file_paths{std::move(t_file_paths)}, m_defines{std::move(t_defines)}
    {
    }

    std::string shader_processor::process_shader(const std::filesystem::path& shader_file)
    {
        std::size_t file_id = 0;
        return process_shader_recursive(shader_file, file_id, 0);
    }

    std::filesystem::path shader_processor::find_file_location(const std::filesystem::path& relative_path)
    {
        for (const auto& dir : m_file_paths) {
            auto filename = dir / relative_path;
            if (dir.empty()) { filename = relative_path; }
            if (std::filesystem::exists(filename)) { return filename; }
        }

        spdlog::critical("Cannot find local resource file \"{}\".", relative_path.string());
        throw std::runtime_error(fmt::format("Cannot find local resource file ({}).", relative_path.string()));
    }

    std::string shader_processor::process_shader_recursive(const std::filesystem::path& shader_file,
                                                           std::size_t& file_id, std::size_t recursion_depth)
    {
        if (recursion_depth > max_shader_recursion_depth) {
            spdlog::critical("Header inclusion depth limit reached! Cyclic header inclusion? File: {}",
                             shader_file.string());
            throw std::runtime_error(fmt::format(
                "Header inclusion depth limit reached! Cyclic header inclusion? File: {}", shader_file.string()));
        }
        namespace filesystem = std::filesystem;

        std::ifstream file(shader_file, std::ifstream::in);
        std::string line;
        std::string content;
        std::size_t lineCount = 1;
        auto next_file_id = file_id + 1;

        while (file.good()) {
            std::getline(file, line);
            auto trimed_line = trim_copy(line);

            static const std::regex include_regex(R"(^[ ]*#[ ]*include[ ]+["<](.*)[">].*)");
            std::smatch include_matches;
            if (std::regex_search(line, include_matches, include_regex)) {
                filesystem::path relative_filename{shader_file.parent_path() / include_matches[1].str()};
                auto include_file = find_file_location(relative_filename);
                if (!filesystem::exists(include_file)) {
                    spdlog::critical("{}({}): fatal error: cannot open include file \"{}\".", shader_file.string(),
                                     lineCount, include_file.string());
                    throw std::runtime_error(fmt::format("{}({}): fatal error: cannot open include file \"{}\".",
                                                         shader_file.string(), lineCount, include_file.string()));
                }
                content.append(fmt::format("#line {} {}\n", 1, next_file_id));
                content.append(process_shader_recursive(include_file, next_file_id, recursion_depth + 1));
                content.append(fmt::format("#line {} {}\n", lineCount + 1, file_id));
            } else {
                content.append(line).append("\n");
            }

            if (trimed_line.starts_with("#version")) {
                for (const auto& def : m_defines) {
                    auto trimedDefine = trim_copy(def);
                    content.append(fmt::format("#define {}\n", trimedDefine));
                }
                content.append(fmt::format("#line {} {}\n", lineCount + 1, file_id));
            }
            ++lineCount;
        }

        file.close();
        file_id = next_file_id;
        return content;
    }
}
