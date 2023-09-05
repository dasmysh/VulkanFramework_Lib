/**
 * @file   main.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.03.29
 *
 * @brief  Implements the main for the GLSL shader preprocessor.
 */

#include "constants.h"
#include "shader_preprocess.h"

#include <docopt/docopt.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <iostream>
#include <filesystem>
#include <fstream>

constexpr const char* program_usage = R"(Vulkan framework GLSL shader preprocessor.
Usage:
    vkfw_glsl_preprocessor <input_files> ... [-i <include_directory>]... -o <output_file>
    vkfw_glsl_preprocessor (-h | --help)
    vkfw_glsl_preprocessor --version

Arguments:
    <input_file>            the GLSL file(s) to preprocess

Options:
    -h --help               show this
    --version               show version
    -i <include_directory>  (base) directories to search for include files
    -o <output_file>        the output file
)";


int main(int argc, const char** argv)
{
    try {
        const std::string directory;
        const std::string name = vkfw_glsl::logFileName;

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::warn);
        console_sink->set_pattern(fmt::format("[{}] [%^%l%$] %v", vkfw_glsl::logTag));

        auto devenv_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
        devenv_sink->set_level(spdlog::level::err);
        devenv_sink->set_pattern(fmt::format("[{}] [%^%l%$] %v", vkfw_glsl::logTag));

        auto file_sink =
            std::make_shared<spdlog::sinks::basic_file_sink_mt>(directory.empty() ? name : directory + "/" + name, 5);
        file_sink->set_level(spdlog::level::trace);

        spdlog::sinks_init_list sink_list = {file_sink, console_sink, devenv_sink};
        auto logger = std::make_shared<spdlog::logger>(vkfw_glsl::logTag, sink_list.begin(), sink_list.end());

        spdlog::set_default_logger(logger);
        spdlog::flush_on(spdlog::level::err);

        if constexpr (vkfw_glsl::debug_build) {
            spdlog::set_level(spdlog::level::trace);
        } else {
            spdlog::set_level(spdlog::level::err);
        }

        spdlog::info("Log created.");

    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        return 1;
    }
    std::copy(argv, argv + argc, std::ostream_iterator<const char*>(std::cout, " "));

    try {
        std::map<std::string, docopt::value> args =
            docopt::docopt(program_usage, {argv + 1, argv + argc}, true, "VKFW GLSL Preprocessor 0.1"); // NOLINT

        std::vector<std::string> input_filenames = args["<input_files>"].asStringList();
        std::string output_filename = args["-o"].asString();
        std::vector<std::string> include_directory_names = args["-i"].asStringList();

        std::vector<std::filesystem::path> input_files;
        input_files.reserve(input_filenames.size());
        for (const auto& input_filename : input_filenames) { input_files.emplace_back(input_filename); }

        std::vector<std::filesystem::path> include_directories;
        include_directories.reserve(include_directory_names.size());
        for (const auto& include_directory_name : include_directory_names) {
            include_directories.emplace_back(include_directory_name);
        }

        vkfw_glsl::shader_processor processor{include_directories};

        for (const auto& input_file : input_files) {
            spdlog::info("Processing {}.", input_file.string());
            auto [output_file_content, output_dependency_file_content] = processor.process_shader(input_file);
            std::ofstream output_file(output_filename);
            output_file << output_file_content;
            spdlog::info("Written {}.", output_filename);

            std::ofstream output_dependency_file(output_filename + ".dep");
            output_file << output_dependency_file_content;
        }
    } catch (...) {
        spdlog::critical("Could not process given files. Unknown exception.");
        return 1;
    }

    return 0;
}
