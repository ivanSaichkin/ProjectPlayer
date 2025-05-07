#pragma once

#include <string>
#include <map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace VideoPlayer {
namespace Storage {

class ConfigManager {
public:
    // Singleton pattern
    static ConfigManager& getInstance();

    // Configuration operations
    template<typename T>
    T getValue(const std::string& key, const T& defaultValue) const;

    template<typename T>
    void setValue(const std::string& key, const T& value);

    bool hasKey(const std::string& key) const;
    void removeKey(const std::string& key);
    void clear();

    // File operations
    bool loadConfig(const std::string& filename = "");
    bool saveConfig(const std::string& filename = "");

    // Set default configuration path
    void setConfigPath(const std::string& path);
    std::string getConfigPath() const;

private:
    ConfigManager();
    ~ConfigManager();
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    json config;
    std::string configPath;
};

// Template specializations for different types
template<>
inline std::string ConfigManager::getValue<std::string>(const std::string& key, const std::string& defaultValue) const {
    if (config.contains(key)) {
        return config[key].get<std::string>();
    }
    return defaultValue;
}

template<>
inline int ConfigManager::getValue<int>(const std::string& key, const int& defaultValue) const {
    if (config.contains(key)) {
        return config[key].get<int>();
    }
    return defaultValue;
}

template<>
inline double ConfigManager::getValue<double>(const std::string& key, const double& defaultValue) const {
    if (config.contains(key)) {
        return config[key].get<double>();
    }
    return defaultValue;
}

template<>
inline bool ConfigManager::getValue<bool>(const std::string& key, const bool& defaultValue) const {
    if (config.contains(key)) {
        return config[key].get<bool>();
    }
    return defaultValue;
}

// General implementation for setValue
template<typename T>
inline void ConfigManager::setValue(const std::string& key, const T& value) {
    config[key] = value;
}

} // namespace Storage
} // namespace VideoPlayer
