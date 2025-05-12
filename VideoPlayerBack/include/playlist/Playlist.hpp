#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace VideoPlayer {
namespace Playlist {

struct PlaylistItem {
    std::string path;
    std::string title;
    double duration;
    std::string thumbnailPath;
    double lastPosition;  // Last playback position in seconds
};

// JSON serialization for PlaylistItem
inline void to_json(json& j, const PlaylistItem& item) {
    j = json{
        {"path",          item.path         },
        {"title",         item.title        },
        {"duration",      item.duration     },
        {"thumbnailPath", item.thumbnailPath},
        {"lastPosition",  item.lastPosition }
    };
}

inline void from_json(const json& j, PlaylistItem& item) {
    j.at("path").get_to(item.path);
    j.at("title").get_to(item.title);
    j.at("duration").get_to(item.duration);
    j.at("thumbnailPath").get_to(item.thumbnailPath);
    j.at("lastPosition").get_to(item.lastPosition);
}

class Playlist {
 public:
    Playlist(const std::string& name = "New Playlist");
    ~Playlist();

    // Basic operations
    void addItem(const std::string& path, const std::string& title = "");
    void removeItem(size_t index);
    void moveItem(size_t fromIndex, size_t toIndex);
    void clear();

    // Getters and setters
    std::string getName() const;
    void setName(const std::string& name);
    size_t getItemCount() const;
    PlaylistItem getItem(size_t index) const;
    std::vector<PlaylistItem>& getItems();
    const std::vector<PlaylistItem>& getItems() const;

    // Update item information
    void updateItemDuration(size_t index, double duration);
    void updateItemPosition(size_t index, double position);
    void updateItemThumbnail(size_t index, const std::string& thumbnailPath);

    // File operations
    bool saveToFile(const std::string& filename);
    bool loadFromFile(const std::string& filename);

    // Convert to/from JSON
    json toJson() const;
    bool fromJson(const json& j);

 private:
    std::string name;
    std::vector<PlaylistItem> items;

    // Generate thumbnail for video file
    std::string generateThumbnail(const std::string& videoPath);
};

}  // namespace Playlist
}  // namespace VideoPlayer
