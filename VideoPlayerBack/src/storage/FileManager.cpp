#include "../../include/storage/FileManager.hpp"

#include <algorithm>
#include <fstream>

#include "../../include/core/ErrorHandler.hpp"

namespace VideoPlayer {
namespace Storage {

FileManager& FileManager::getInstance() {
    static FileManager instance;
    return instance;
}

FileManager::FileManager() {
    // Set up media extensions
    mediaExtensions = {".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", ".webm", ".m4v", ".mpg", ".mpeg",
                       ".3gp", ".3g2", ".mp3", ".wav", ".ogg", ".aac", ".flac", ".wma", ".m4a"};

    // Set up directories
    try {
        // Get application directory
        appDirectory = fs::current_path().string();

        // Set data directory
        dataDirectory = appDirectory + "/data";

        // Create data directory if it doesn't exist
        if (!fs::exists(dataDirectory)) {
            fs::create_directories(dataDirectory);
        }

        // Load recent directories
        loadRecentDirectories();
    } catch (const std::exception& e) {
        Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR,
                                                      "Error initializing FileManager: " + std::string(e.what()));
    }
}

FileManager::~FileManager() {
    // Save recent directories
    saveRecentDirectories();
}

std::vector<FileInfo> FileManager::listDirectory(const std::string& path) {
    std::vector<FileInfo> result;

    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            FileInfo info;
            info.path = entry.path().string();
            info.name = entry.path().filename().string();
            info.extension = entry.path().extension().string();
            info.isDirectory = entry.is_directory();

            if (!info.isDirectory) {
                info.size = entry.file_size();
            } else {
                info.size = 0;
            }

            auto lastWriteTime = fs::last_write_time(entry.path());
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(lastWriteTime - fs::file_time_type::clock::now() +
                                                                                          std::chrono::system_clock::now());
            info.lastModified = sctp;

            result.push_back(info);
        }

        // Add to recent directories
        addRecentDirectory(path);
    } catch (const std::exception& e) {
        Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR, "Error listing directory: " + std::string(e.what()));
    }

    return result;
}

std::vector<FileInfo> FileManager::findMediaFiles(const std::string& directory, bool recursive) {
    std::vector<FileInfo> result;

    try {
        if (recursive) {
            for (const auto& entry : fs::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file() && isMediaFile(entry.path().string())) {
                    result.push_back(getFileInfo(entry.path().string()));
                }
            }
        } else {
            for (const auto& entry : fs::directory_iterator(directory)) {
                if (entry.is_regular_file() && isMediaFile(entry.path().string())) {
                    result.push_back(getFileInfo(entry.path().string()));
                }
            }
        }

        // Add to recent directories
        addRecentDirectory(directory);
    } catch (const std::exception& e) {
        Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR,
                                                      "Error finding media files: " + std::string(e.what()));
    }

    return result;
}

bool FileManager::createDirectory(const std::string& path) {
    try {
        return fs::create_directories(path);
    } catch (const std::exception& e) {
        Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR,
                                                      "Error creating directory: " + std::string(e.what()));
        return false;
    }
}

bool FileManager::removeFile(const std::string& path) {
    try {
        return fs::remove(path);
    } catch (const std::exception& e) {
        Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR, "Error removing file: " + std::string(e.what()));
        return false;
    }
}

bool FileManager::copyFile(const std::string& source, const std::string& destination) {
    try {
        fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception& e) {
        Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR, "Error copying file: " + std::string(e.what()));
        return false;
    }
}

bool FileManager::moveFile(const std::string& source, const std::string& destination) {
    try {
        fs::rename(source, destination);
        return true;
    } catch (const std::exception& e) {
        Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR, "Error moving file: " + std::string(e.what()));
        return false;
    }
}

std::string FileManager::getApplicationDirectory() const {
    return appDirectory;
}

std::string FileManager::getDataDirectory() const {
    return dataDirectory;
}

std::string FileManager::getConfigDirectory() const {
    return dataDirectory;
}

std::string FileManager::getPlaylistDirectory() const {
    std::string playlistDir = dataDirectory + "/playlists";

    // Create directory if it doesn't exist
    if (!fs::exists(playlistDir)) {
        try {
            fs::create_directories(playlistDir);
        } catch (const std::exception& e) {
            Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR,
                                                          "Error creating playlist directory: " + std::string(e.what()));
        }
    }

    return playlistDir;
}

FileInfo FileManager::getFileInfo(const std::string& path) {
    FileInfo info;

    try {
        fs::path fsPath(path);
        info.path = path;
        info.name = fsPath.filename().string();
        info.extension = fsPath.extension().string();
        info.isDirectory = fs::is_directory(path);

        if (!info.isDirectory) {
            info.size = fs::file_size(path);
        } else {
            info.size = 0;
        }

        auto lastWriteTime = fs::last_write_time(path);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(lastWriteTime - fs::file_time_type::clock::now() +
                                                                                      std::chrono::system_clock::now());
        info.lastModified = sctp;
    } catch (const std::exception& e) {
        Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR, "Error getting file info: " + std::string(e.what()));
    }

    return info;
}

bool FileManager::isMediaFile(const std::string& path) {
    std::string ext = getFileExtension(path);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    for (const auto& mediaExt : mediaExtensions) {
        if (ext == mediaExt) {
            return true;
        }
    }

    return false;
}

std::string FileManager::getFileExtension(const std::string& path) {
    fs::path fsPath(path);
    return fsPath.extension().string();
}

std::string FileManager::getFileName(const std::string& path) {
    fs::path fsPath(path);
    return fsPath.filename().string();
}

void FileManager::addRecentDirectory(const std::string& path) {
    // Remove if already exists
    recentDirectories.erase(std::remove(recentDirectories.begin(), recentDirectories.end(), path), recentDirectories.end());

    // Add to front
    recentDirectories.insert(recentDirectories.begin(), path);

    // Keep only the last 10 directories
    if (recentDirectories.size() > 10) {
        recentDirectories.resize(10);
    }
}

std::vector<std::string> FileManager::getRecentDirectories() const {
    return recentDirectories;
}

void FileManager::clearRecentDirectories() {
    recentDirectories.clear();
}

bool FileManager::saveRecentDirectories(const std::string& filename) {
    std::string path;

    if (filename.empty()) {
        path = dataDirectory + "/recent_directories.json";
    } else {
        path = filename;
    }

    try {
        std::ofstream file(path);
        if (!file.is_open()) {
            Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR, "Failed to open file for writing: " + path);
            return false;
        }

        file << toJson().dump(4);
        return true;
    } catch (const std::exception& e) {
        Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR,
                                                      "Error saving recent directories: " + std::string(e.what()));
        return false;
    }
}

bool FileManager::loadRecentDirectories(const std::string& filename) {
    std::string path;

    if (filename.empty()) {
        path = dataDirectory + "/recent_directories.json";
    } else {
        path = filename;
    }

    try {
        // Check if file exists
        if (!fs::exists(path)) {
            return true;
        }

        std::ifstream file(path);
        if (!file.is_open()) {
            Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR, "Failed to open file for reading: " + path);
            return false;
        }

        json j;
        file >> j;
        return fromJson(j);
    } catch (const std::exception& e) {
        Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR,
                                                      "Error loading recent directories: " + std::string(e.what()));
        return false;
    }
}

json FileManager::toJson() const {
    json j;
    j["recentDirectories"] = recentDirectories;
    return j;
}

bool FileManager::fromJson(const json& j) {
    try {
        if (j.contains("recentDirectories") && j["recentDirectories"].is_array()) {
            recentDirectories = j["recentDirectories"].get<std::vector<std::string>>();
        }
        return true;
    } catch (const std::exception& e) {
        Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR,
                                                      "Error parsing recent directories JSON: " + std::string(e.what()));
        return false;
    }
}

}  // namespace Storage
}  // namespace VideoPlayer
