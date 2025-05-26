#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <sstream>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace VideoPlayer {
namespace Storage {

struct FileInfo {
    std::string path;
    std::string name;
    std::string extension;
    uint64_t size;
    std::chrono::system_clock::time_point lastModified;
    bool isDirectory;
};

// JSON serialization for FileInfo
inline void to_json(json& j, const FileInfo& info) {
    auto time_t = std::chrono::system_clock::to_time_t(info.lastModified);
    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%FT%T%z", std::localtime(&time_t));

    j = json{
        {"path",         info.path       },
        {"name",         info.name       },
        {"extension",    info.extension  },
        {"size",         info.size       },
        {"lastModified", timeStr         },
        {"isDirectory",  info.isDirectory}
    };
}

inline void from_json(const json& j, FileInfo& info) {
    j.at("path").get_to(info.path);
    j.at("name").get_to(info.name);
    j.at("extension").get_to(info.extension);
    j.at("size").get_to(info.size);

    std::string timeStr = j.at("lastModified").get<std::string>();
    std::tm tm = {};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    info.lastModified = std::chrono::system_clock::from_time_t(std::mktime(&tm));

    j.at("isDirectory").get_to(info.isDirectory);
}

class FileManager {
 public:
    // Singleton pattern
    static FileManager& getInstance();

    // File operations
    std::vector<FileInfo> listDirectory(const std::string& path);
    std::vector<FileInfo> findMediaFiles(const std::string& directory, bool recursive = false);
    bool createDirectory(const std::string& path);
    bool removeFile(const std::string& path);
    bool copyFile(const std::string& source, const std::string& destination);
    bool moveFile(const std::string& source, const std::string& destination);

    // Path operations
    std::string getApplicationDirectory() const;
    std::string getDataDirectory() const;
    std::string getConfigDirectory() const;
    std::string getPlaylistDirectory() const;

    // File info
    FileInfo getFileInfo(const std::string& path);
    bool isMediaFile(const std::string& path);
    std::string getFileExtension(const std::string& path);
    std::string getFileName(const std::string& path);

    // Recent directories
    void addRecentDirectory(const std::string& path);
    std::vector<std::string> getRecentDirectories() const;
    void clearRecentDirectories();

    // Save/load recent directories
    bool saveRecentDirectories(const std::string& filename = "");
    bool loadRecentDirectories(const std::string& filename = "");

 private:
    FileManager();
    ~FileManager();
    FileManager(const FileManager&) = delete;
    FileManager& operator=(const FileManager&) = delete;

    std::vector<std::string> recentDirectories;
    std::string appDirectory;
    std::string dataDirectory;
    std::vector<std::string> mediaExtensions;

    // Helper methods
    json toJson() const;
    bool fromJson(const json& j);
};

}  // namespace Storage
}  // namespace VideoPlayer
