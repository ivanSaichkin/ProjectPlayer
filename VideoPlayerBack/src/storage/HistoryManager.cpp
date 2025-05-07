#include "../../include/storage/HistoryManager.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include "../../include/core/ErrorHandler.hpp"

namespace fs = std::filesystem;
namespace VideoPlayer {
namespace Storage {

HistoryManager& HistoryManager::getInstance() {
    static HistoryManager instance;
    return instance;
}

HistoryManager::HistoryManager() : historyPath("data/history.json") {
    // Load history on creation
    loadHistory();
}

HistoryManager::~HistoryManager() {
    // Save history on destruction
    saveHistory();
}

void HistoryManager::addEntry(const std::string& filePath, const std::string& title) {
    // Check if entry already exists
    for (auto& entry : history) {
        if (entry.filePath == filePath) {
            // Update existing entry
            entry.title = title;
            entry.lastPlayed = std::chrono::system_clock::now();
            entry.playCount++;
            return;
        }
    }

    // Create new entry
    HistoryEntry entry;
    entry.filePath = filePath;
    entry.title = title;
    entry.lastPlayed = std::chrono::system_clock::now();
    entry.lastPosition = 0.0;
    entry.playCount = 1;

    history.push_back(entry);
}

void HistoryManager::updateEntry(const std::string& filePath, double position) {
    for (auto& entry : history) {
        if (entry.filePath == filePath) {
            entry.lastPosition = position;
            entry.lastPlayed = std::chrono::system_clock::now();
            return;
        }
    }
}

void HistoryManager::removeEntry(const std::string& filePath) {
    history.erase(
        std::remove_if(history.begin(), history.end(),
            [&filePath](const HistoryEntry& entry) { return entry.filePath == filePath; }),
        history.end());
}

void HistoryManager::clearHistory() {
    history.clear();
}

std::vector<HistoryEntry> HistoryManager::getRecentEntries(size_t count) const {
    // Sort by last played time (most recent first)
    std::vector<HistoryEntry> sorted = history;
    std::sort(sorted.begin(), sorted.end(),
        [](const HistoryEntry& a, const HistoryEntry& b) {
            return a.lastPlayed > b.lastPlayed;
        });

    // Return the requested number of entries
    if (sorted.size() > count) {
        sorted.resize(count);
    }

    return sorted;
}

std::vector<HistoryEntry> HistoryManager::getMostPlayedEntries(size_t count) const {
    // Sort by play count (most played first)
    std::vector<HistoryEntry> sorted = history;
    std::sort(sorted.begin(), sorted.end(),
        [](const HistoryEntry& a, const HistoryEntry& b) {
            return a.playCount > b.playCount;
        });

    // Return the requested number of entries
    if (sorted.size() > count) {
        sorted.resize(count);
    }

    return sorted;
}

HistoryEntry HistoryManager::getEntry(const std::string& filePath) const {
    for (const auto& entry : history) {
        if (entry.filePath == filePath) {
            return entry;
        }
    }

    // Return empty entry if not found
    return HistoryEntry();
}

bool HistoryManager::hasEntry(const std::string& filePath) const {
    for (const auto& entry : history) {
        if (entry.filePath == filePath) {
            return true;
        }
    }

    return false;
}

bool HistoryManager::saveHistory(const std::string& filename) {
    std::string path = filename.empty() ? historyPath : filename;

    try {
        // Create directory if it doesn't exist
        fs::path dirPath = fs::path(path).parent_path();
        if (!dirPath.empty() && !fs::exists(dirPath)) {
            fs::create_directories(dirPath);
        }

        std::ofstream file(path);
        if (!file.is_open()) {
            ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to open history file for writing: " + path);
            return false;
        }

        file << toJson().dump(4); // Pretty print with 4-space indentation
        return true;
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Error saving history: " + std::string(e.what()));
        return false;
    }
}

bool HistoryManager::loadHistory(const std::string& filename) {
    std::string path = filename.empty() ? historyPath : filename;

    try {
        // Check if file exists
        if (!fs::exists(path)) {
            // Not an error, just use empty history
            return true;
        }

        std::ifstream file(path);
        if (!file.is_open()) {
            ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to open history file: " + path);
            return false;
        }

        json j;
        file >> j;
        return fromJson(j);
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Error loading history: " + std::string(e.what()));
        return false;
    }
}

void HistoryManager::setHistoryPath(const std::string& path) {
    historyPath = path;
}

std::string HistoryManager::getHistoryPath() const {
    return historyPath;
}

json HistoryManager::toJson() const {
    json j = json::array();

    for (const auto& entry : history) {
        j.push_back(entry);
    }

    return j;
}

bool HistoryManager::fromJson(const json& j) {
    try {
        history.clear();

        if (!j.is_array()) {
            return false;
        }

        for (const auto& entryJson : j) {
            HistoryEntry entry = entryJson.get<HistoryEntry>();
            history.push_back(entry);
        }

        return true;
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Error parsing history JSON: " + std::string(e.what()));
        return false;
    }
}

} // namespace Storage
} // namespace VideoPlayer
