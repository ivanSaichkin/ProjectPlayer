#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace VideoPlayer {
namespace Storage {

struct HistoryEntry {
    std::string filePath;
    std::string title;
    std::chrono::system_clock::time_point lastPlayed;
    double lastPosition;
    int playCount;
};

// JSON serialization for HistoryEntry
inline void to_json(json& j, const HistoryEntry& entry) {
    // Convert time_point to ISO string
    auto time_t = std::chrono::system_clock::to_time_t(entry.lastPlayed);
    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%FT%T%z", std::localtime(&time_t));

    j = json{
        {"filePath", entry.filePath},
        {"title", entry.title},
        {"lastPlayed", timeStr},
        {"lastPosition", entry.lastPosition},
        {"playCount", entry.playCount}
    };
}

inline void from_json(const json& j, HistoryEntry& entry) {
    j.at("filePath").get_to(entry.filePath);
    j.at("title").get_to(entry.title);

    // Parse ISO string to time_point
    std::string timeStr = j.at("lastPlayed").get<std::string>();
    std::tm tm = {};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    entry.lastPlayed = std::chrono::system_clock::from_time_t(std::mktime(&tm));

    j.at("lastPosition").get_to(entry.lastPosition);
    j.at("playCount").get_to(entry.playCount);
}

class HistoryManager {
public:
    // Singleton pattern
    static HistoryManager& getInstance();

    // History operations
    void addEntry(const std::string& filePath, const std::string& title);
    void updateEntry(const std::string& filePath, double position);
    void removeEntry(const std::string& filePath);
    void clearHistory();

    // Getters
    std::vector<HistoryEntry> getRecentEntries(size_t count = 10) const;
    std::vector<HistoryEntry> getMostPlayedEntries(size_t count = 10) const;
    HistoryEntry getEntry(const std::string& filePath) const;
    bool hasEntry(const std::string& filePath) const;

    // File operations
    bool saveHistory(const std::string& filename = "");
    bool loadHistory(const std::string& filename = "");

    // Set default history file path
    void setHistoryPath(const std::string& path);
    std::string getHistoryPath() const;

private:
    HistoryManager();
    ~HistoryManager();
    HistoryManager(const HistoryManager&) = delete;
    HistoryManager& operator=(const HistoryManager&) = delete;

    std::vector<HistoryEntry> history;
    std::string historyPath;

    // Helper methods
    json toJson() const;
    bool fromJson(const json& j);
};

} // namespace Storage
} // namespace VideoPlayer
