#include "../../include/playlist/Playlist.hpp"

#include <filesystem>
#include <fstream>

#include "../../include/core/ErrorHandler.hpp"

namespace fs = std::filesystem;
namespace VideoPlayer {
namespace Playlist {

Playlist::Playlist(const std::string& name) : name(name) {
}

Playlist::~Playlist() {
}

void Playlist::addItem(const std::string& path, const std::string& title) {
    PlaylistItem item;
    item.path = path;

    // Use provided title or extract from filename
    if (title.empty()) {
        fs::path filePath(path);
        item.title = filePath.filename().string();
    } else {
        item.title = title;
    }

    // Initialize other fields
    item.duration = 0.0;
    item.thumbnailPath = "";
    item.lastPosition = 0.0;

    // Generate thumbnail asynchronously (in a real implementation)
    item.thumbnailPath = generateThumbnail(path);

    items.push_back(item);
}

void Playlist::removeItem(size_t index) {
    if (index < items.size()) {
        items.erase(items.begin() + index);
    }
}

void Playlist::moveItem(size_t fromIndex, size_t toIndex) {
    if (fromIndex < items.size() && toIndex < items.size() && fromIndex != toIndex) {
        PlaylistItem item = items[fromIndex];
        items.erase(items.begin() + fromIndex);

        if (fromIndex < toIndex) {
            items.insert(items.begin() + toIndex - 1, item);
        } else {
            items.insert(items.begin() + toIndex, item);
        }
    }
}

void Playlist::clear() {
    items.clear();
}

std::string Playlist::getName() const {
    return name;
}

void Playlist::setName(const std::string& name) {
    this->name = name;
}

size_t Playlist::getItemCount() const {
    return items.size();
}

PlaylistItem Playlist::getItem(size_t index) const {
    if (index < items.size()) {
        return items[index];
    }
    return PlaylistItem();
}

std::vector<PlaylistItem>& Playlist::getItems() {
    return items;
}

const std::vector<PlaylistItem>& Playlist::getItems() const {
    return items;
}

void Playlist::updateItemDuration(size_t index, double duration) {
    if (index < items.size()) {
        items[index].duration = duration;
    }
}

void Playlist::updateItemPosition(size_t index, double position) {
    if (index < items.size()) {
        items[index].lastPosition = position;
    }
}

void Playlist::updateItemThumbnail(size_t index, const std::string& thumbnailPath) {
    if (index < items.size()) {
        items[index].thumbnailPath = thumbnailPath;
    }
}

bool Playlist::saveToFile(const std::string& filename) {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR, "Failed to open file for writing: " + filename);
            return false;
        }

        json j = toJson();
        file << j.dump(4);  // Pretty print with 4-space indentation
        return true;
    } catch (const std::exception& e) {
        Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR, "Error saving playlist: " + std::string(e.what()));
        return false;
    }
}

bool Playlist::loadFromFile(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR, "Failed to open file for reading: " + filename);
            return false;
        }

        json j;
        file >> j;
        return fromJson(j);
    } catch (const std::exception& e) {
        Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR, "Error loading playlist: " + std::string(e.what()));
        return false;
    }
}

json Playlist::toJson() const {
    json j;
    j["name"] = name;
    j["items"] = items;  // Uses the to_json function for PlaylistItem
    return j;
}

bool Playlist::fromJson(const json& j) {
    try {
        name = j.at("name").get<std::string>();
        items = j.at("items").get<std::vector<PlaylistItem>>();
        return true;
    } catch (const std::exception& e) {
        Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR,
                                                      "Error parsing playlist JSON: " + std::string(e.what()));
        return false;
    }
}

std::string Playlist::generateThumbnail(const std::string& videoPath) {
    // This would use a thumbnail generation utility
    // For now, return an empty string
    return "";
}

}  // namespace Playlist
}  // namespace VideoPlayer
