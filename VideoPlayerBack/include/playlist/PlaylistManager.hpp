#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "Playlist.hpp"

using json = nlohmann::json;

namespace VideoPlayer {
namespace Playlist {

class PlaylistManager {
public:
    // Singleton pattern
    static PlaylistManager& getInstance();

    // Playlist operations
    std::shared_ptr<Playlist> createPlaylist(const std::string& name = "New Playlist");
    bool loadPlaylist(const std::string& filename);
    bool savePlaylist(std::shared_ptr<Playlist> playlist, const std::string& filename = "");
    void deletePlaylist(const std::string& name);

    // Playlist getters
    std::shared_ptr<Playlist> getPlaylist(const std::string& name) const;
    std::shared_ptr<Playlist> getCurrentPlaylist() const;
    std::vector<std::string> getPlaylistNames() const;

    // Current playlist management
    void setCurrentPlaylist(std::shared_ptr<Playlist> playlist);
    void setCurrentPlaylist(const std::string& name);

    // Current item management
    bool setCurrentItem(size_t index);
    size_t getCurrentItemIndex() const;
    PlaylistItem getCurrentItem() const;
    bool hasNextItem() const;
    bool hasPreviousItem() const;
    bool playNextItem();
    bool playPreviousItem();

    // Serialization
    json toJson() const;
    bool fromJson(const json& j);

    // File operations
    bool saveState(const std::string& filename = "");
    bool loadState(const std::string& filename = "");

private:
    PlaylistManager();
    ~PlaylistManager();
    PlaylistManager(const PlaylistManager&) = delete;
    PlaylistManager& operator=(const PlaylistManager&) = delete;

    std::vector<std::shared_ptr<Playlist>> playlists;
    std::shared_ptr<Playlist> currentPlaylist;
    size_t currentItemIndex;
    std::string defaultDirectory;
};

} // namespace Playlist
} // namespace VideoPlayer
