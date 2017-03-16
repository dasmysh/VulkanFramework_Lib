/**
 * @file   rotfilesink.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.28
 *
 * @brief  Implementation of a derivation of FileSink for g3log using a rotation principle.
 * This file is heavily based on the FileSink by KjellKod.cc.
 */

#include "rotfilesink.h"
#include "filesinkhelper.inl"
#include <cassert>
#include <filesystem>
#include <regex>

namespace vku {

    std::string createLogFileName(const std::string& _log_prefix_backup, const std::string& logger_id, unsigned int rotSize)
    {
        auto file_name = _log_prefix_backup;
        if (logger_id != "") file_name += "." + logger_id;
        auto complete_file_name = file_name + ".log";

        namespace fs = std::experimental::filesystem;
        auto complete_file_name_path(fs::absolute(complete_file_name));
        if (fs::exists(complete_file_name_path)) {
            // std::vector<std::pair<fs::path, std::smatch>> oldLogs;
            std::vector<fs::path> oldLogs;
            std::regex fn_regex(R"(^)" + file_name + R"(.(\d+).log$)");

            fs::recursive_directory_iterator it(complete_file_name_path.parent_path());
            fs::recursive_directory_iterator endit;

            while (it != endit) {
                std::smatch match; auto fnToMatch = it->path().filename().string();
                auto matched = std::regex_match(fnToMatch, match, fn_regex);
                // if (fs::is_regular_file(*it) && matched) oldLogs.push_back(std::make_pair(it->path(), match));
                if (fs::is_regular_file(*it) && matched) oldLogs.push_back(it->path());
                ++it;
            }

            std::sort(oldLogs.begin(), oldLogs.end());
            for (auto i = static_cast<int>(oldLogs.size()) - 1; i >= 0; --i) {
                if (i < static_cast<int>(rotSize) - 1) {
                    std::stringstream new_file_name;
                    new_file_name << file_name << "." << i + 1 << ".log";
                    fs::path new_file_path(new_file_name.str());
                    // fs::rename(oldLogs[i].first, new_file_path);
                    fs::rename(oldLogs[i], new_file_path);
                // } else fs::remove(oldLogs[i].first);
                } else fs::remove(oldLogs[i]);
            }

            fs::rename(complete_file_name_path, fs::path(file_name + ".0.log"));
        }
        // check if exists
        // if so: find all with pattern, just keep newest rotSize, rename

        return complete_file_name;
    }

    RotationFileSink::RotationFileSink(const std::string &log_prefix, const std::string &log_directory, unsigned int rotSize, const std::string& logger_id)
        : rotationSize_{ rotSize },
        _log_file_with_path(log_directory)
        , _log_prefix_backup(log_prefix)
        , _outptr(new std::ofstream)
    {
        _log_prefix_backup = internal::prefixSanityFix(log_prefix);
        if (!internal::isValidFilename(_log_prefix_backup)) {
            std::cerr << "g3log: forced abort due to illegal log prefix [" << log_prefix << "]" << std::endl;
            abort();
        }

        auto file_name = createLogFileName(_log_prefix_backup, logger_id, rotationSize_);
        _log_file_with_path = internal::pathSanityFix(_log_file_with_path, file_name);
        _outptr = internal::createLogFile(_log_file_with_path);

        if (!_outptr) {
            std::cerr << "Cannot write log file to location, attempting current directory" << std::endl;
            _log_file_with_path = "./" + file_name;
            _outptr = internal::createLogFile(_log_file_with_path);
        }
        assert(_outptr && "cannot open log file at startup");
        addLogFileHeader();
    }


    RotationFileSink::~RotationFileSink() {
        std::string exit_msg{ "\ng3log g3FileSink shutdown at: " };
        exit_msg.append(g3::localtime_formatted(g3::systemtime_now(), g3::internal::time_formatted));
        filestream() << exit_msg << std::flush;

        exit_msg.append({ "\nLog file at: [" }).append(_log_file_with_path).append({ "]\n\n" });
        std::cerr << exit_msg << std::flush;
    }

    // The actual log receiving function
    void RotationFileSink::fileWrite(g3::LogMessageMover message) const
    {
        auto& out(filestream());
        out << message.get().toString() << std::flush;
    }

    std::string RotationFileSink::changeLogFile(const std::string &directory, const std::string &logger_id) {

        auto now = g3::systemtime_now();
        auto now_formatted = g3::localtime_formatted(now, { g3::internal::date_formatted + " " + g3::internal::time_formatted });

        auto file_name = createLogFileName(_log_prefix_backup, logger_id, rotationSize_);
        auto prospect_log = directory + file_name;
        auto log_stream = internal::createLogFile(prospect_log);
        if (nullptr == log_stream) {
            filestream() << "\n" << now_formatted << " Unable to change log file. Illegal filename or busy? Unsuccessful log name was: " << prospect_log;
            return{}; // no success
        }

        addLogFileHeader();
        std::ostringstream ss_change;
        ss_change << "\n\tChanging log file from : " << _log_file_with_path;
        ss_change << "\n\tto new location: " << prospect_log << "\n";
        filestream() << now_formatted << ss_change.str();
        ss_change.str("");

        auto old_log = _log_file_with_path;
        _log_file_with_path = prospect_log;
        _outptr = std::move(log_stream);
        ss_change << "\n\tNew log file. The previous log file was at: ";
        ss_change << old_log;
        filestream() << now_formatted << ss_change.str();
        return _log_file_with_path;
    }
    std::string RotationFileSink::fileName() const
    {
        return _log_file_with_path;
    }
    void RotationFileSink::addLogFileHeader() const
    {
        filestream() << internal::header();
    }

}
