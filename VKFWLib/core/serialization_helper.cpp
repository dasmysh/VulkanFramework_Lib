#include "serialization_helper.h"
/**
 * @file   serialization_helper.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.05.03
 *
 * @brief  Implementation of serialization helper classes and functions.
 */

#include "serialization_helper.h"
#include <filesystem>

namespace vku {

    BinaryIAWrapper::BinaryIAWrapper(const std::string& filename) :
        ArchiveWrapper{ filename }
    {
        if (IsValid()) {
            auto lastModTime = std::experimental::filesystem::last_write_time(filename);
            auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(lastModTime.time_since_epoch()).count();
            decltype(timestamp) binTimestamp;
            (*this)(cereal::make_nvp("timestamp", binTimestamp));
            if (binTimestamp < timestamp) {
                LOG(WARNING) << "Will not load binary file. Falling back to original." << std::endl
                    << "Filename: " << GetBinFilename(filename) << std::endl
                    << "Description: Timestamp older than original file.";
                Close();
            }
        } else {
            LOG(WARNING) << "Will not load binary file. Falling back to original." << std::endl
                << "Filename: " << GetBinFilename(filename) << std::endl
                << "Description: File does not exist.";
        }
    }

    BinaryOAWrapper::BinaryOAWrapper(const std::string& filename) :
        ArchiveWrapper{ filename }
    {
        auto lastModTime = std::experimental::filesystem::last_write_time(filename);
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(lastModTime.time_since_epoch()).count();
        (*this)(cereal::make_nvp("timestamp", timestamp));
    }
}
