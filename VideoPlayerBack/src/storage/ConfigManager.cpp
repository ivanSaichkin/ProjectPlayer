#include "../../include/storage/ConfigManager.hpp"
#include <fstream>
#include <filesystem>
#include "../../include/core/ErrorHandler.hpp"

namespace fs = std::filesystem;
namespace VideoPlayer {
namespace Storage {

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() : configPath("data/config.json") {
    // Create empty JSON object
    config = json::object();

    // Try to load default config
    loadConfig();
}

ConfigManager::~ConfigManager() {
    // Save config on destruction
    saveConfig();
}

bool ConfigManager::hasKey(const std::string& key) const {
    return config.contains(key);
}

void ConfigManager::removeKey(const std::string& key) {
    if (hasKey(key)) {
        config.erase(key);
    }
}

void ConfigManager::clear() {
    config.clear();
}

bool ConfigManager::loadConfig(const std::string& filename) {
    std::string path = filename.empty() ? configPath : filename;

    try {
        // Check if file exists
        if (!fs::exists(path)) {
            // Not an error, just use default config
            return true;
        }

        std::ifstream file(path);
        if (!file.is_open()) {
            ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to open config file: " + path);
            return false;
        }

        file >> config;
        return true;
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Error loading config: " + std::string(e.what()));
        return false;
    }
}

bool ConfigManager::saveConfig(const std::string& filename) {
    std::string path = filename.empty() ? configPath : filename;

    try {
        // Create directory if it doesn't exist
        fs::path dirPath = fs::path(path).parent_path();
        if (!dirPath.empty() && !fs::exists(dirPath)) {
            fs::create_directories(dirPath);
        }

        std::ofstream file(path);
        if (!file.is_open()) {
            ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to open config file for writing: " + path);
            return false;
        }

        file << config.dump(4); // Pretty print with 4-space indentation
        return true;
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Error saving config: " + std::string(e.what()));
        return false;
    }
}

void ConfigManager::setConfigPath(const std::string& path) {
    configPath = path;
}

std::string ConfigManager::getConfigPath() const {
    return configPath;
}

} // namespace Storage
} // namespace VideoPlayer
