#include "../../include/playlist/PlaylistManager.hpp"

#include <filesystem>
#include <fstream>

#include "../../include/core/ErrorHandler.hpp"
#include "../../include/storage/ConfigManager.hpp"

namespace fs = std::filesystem;
namespace VideoPlayer {
namespace Playlist {

PlaylistManager& PlaylistManager::getInstance() {
    static PlaylistManager instance;
    return instance;
}

PlaylistManager::PlaylistManager() : currentItemIndex(0) {
    // Set default directory from config
    defaultDirectory = Storage::ConfigManager::getInstance().getValue<std::string>("playlistDirectory", "data/playlists");

    // Create default playlist
    createPlaylist("Default");
}

PlaylistManager::~PlaylistManager() {
    // Save state before destruction
    saveState();
}

std::shared_ptr<Playlist> PlaylistManager::createPlaylist(const std::string& name) {
    // Check if playlist with this name already exists
    for (const auto& playlist : playlists) {
        if (playlist->getName() == name) {
            return playlist;
        }
    }

    // Create new playlist
    auto playlist = std::make_shared<Playlist>(name);
    playlists.push_back(playlist);

    // If this is the first playlist, set it as current
    if (playlists.size() == 1) {
        currentPlaylist = playlist;
    }

    return playlist;
}

bool PlaylistManager::loadPlaylist(const std::string& filename) {
    // Create a new playlist
    fs::path path(filename);
    std::string name = path.stem().string();
    auto playlist = std::make_shared<Playlist>(name);

    // Load playlist from file
    if (!playlist->loadFromFile(filename)) {
        return false;
    }

    // Add to playlists
    playlists.push_back(playlist);

    // Set as current playlist
    currentPlaylist = playlist;
    currentItemIndex = 0;

    return true;
}

bool PlaylistManager::savePlaylist(std::shared_ptr<Playlist> playlist, const std::string& filename) {
    std::string path;

    if (filename.empty()) {
        // Use default directory and playlist name
        fs::path dirPath(defaultDirectory);
        fs::path filePath = dirPath / (playlist->getName() + ".json");
        path = filePath.string();

        // Create directory if it doesn't exist
        if (!fs::exists(dirPath)) {
            try {
                fs::create_directories(dirPath);
            } catch (const std::exception& e) {
                Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR,
                                                              "Failed to create playlist directory: " + std::string(e.what()));
                return false;
            }
        }
    } else {
        path = filename;
    }

    return playlist->saveToFile(path);
}

void PlaylistManager::deletePlaylist(const std::string& name) {
    for (auto it = playlists.begin(); it != playlists.end(); ++it) {
        if ((*it)->getName() == name) {
            // If this is the current playlist, set current to null or another playlist
            if (currentPlaylist == *it) {
                if (playlists.size() > 1) {
                    // Set to another playlist
                    if (it == playlists.begin()) {
                        currentPlaylist = *(it + 1);
                    } else {
                        currentPlaylist = *(it - 1);
                    }
                } else {
                    currentPlaylist = nullptr;
                }
                currentItemIndex = 0;
            }

            playlists.erase(it);
            break;
        }
    }
}

std::shared_ptr<Playlist> PlaylistManager::getPlaylist(const std::string& name) const {
    for (const auto& playlist : playlists) {
        if (playlist->getName() == name) {
            return playlist;
        }
    }
    return nullptr;
}

std::shared_ptr<Playlist> PlaylistManager::getCurrentPlaylist() const {
    return currentPlaylist;
}

std::vector<std::string> PlaylistManager::getPlaylistNames() const {
    std::vector<std::string> names;
    for (const auto& playlist : playlists) {
        names.push_back(playlist->getName());
    }
    return names;
}

void PlaylistManager::setCurrentPlaylist(std::shared_ptr<Playlist> playlist) {
    currentPlaylist = playlist;
    currentItemIndex = 0;
}

void PlaylistManager::setCurrentPlaylist(const std::string& name) {
    auto playlist = getPlaylist(name);
    if (playlist) {
        setCurrentPlaylist(playlist);
    }
}

bool PlaylistManager::setCurrentItem(size_t index) {
    if (!currentPlaylist || index >= currentPlaylist->getItemCount()) {
        return false;
    }

    currentItemIndex = index;
    return true;
}

size_t PlaylistManager::getCurrentItemIndex() const {
    return currentItemIndex;
}

PlaylistItem PlaylistManager::getCurrentItem() const {
    if (!currentPlaylist || currentItemIndex >= currentPlaylist->getItemCount()) {
        return PlaylistItem();
    }

    return currentPlaylist->getItem(currentItemIndex);
}

bool PlaylistManager::hasNextItem() const {
    return currentPlaylist && currentItemIndex < currentPlaylist->getItemCount() - 1;
}

bool PlaylistManager::hasPreviousItem() const {
    return currentPlaylist && currentItemIndex > 0;
}

bool PlaylistManager::playNextItem() {
    if (!hasNextItem()) {
        return false;
    }

    currentItemIndex++;
    return true;
}

bool PlaylistManager::playPreviousItem() {
    if (!hasPreviousItem()) {
        return false;
    }

    currentItemIndex--;
    return true;
}

json PlaylistManager::toJson() const {
    json j;

    // Save playlists
    json playlistsJson = json::array();
    for (const auto& playlist : playlists) {
        playlistsJson.push_back(playlist->toJson());
    }
    j["playlists"] = playlistsJson;

    // Save current playlist and item
    if (currentPlaylist) {
        j["currentPlaylist"] = currentPlaylist->getName();
        j["currentItemIndex"] = currentItemIndex;
    }

    return j;
}

bool PlaylistManager::fromJson(const json& j) {
    try {
        // Clear current playlists
        playlists.clear();
        currentPlaylist = nullptr;

        // Load playlists
        if (j.contains("playlists") && j["playlists"].is_array()) {
            for (const auto& playlistJson : j["playlists"]) {
                auto playlist = std::make_shared<Playlist>();
                if (playlist->fromJson(playlistJson)) {
                    playlists.push_back(playlist);
                }
            }
        }

        // Set current playlist
        if (j.contains("currentPlaylist") && j["currentPlaylist"].is_string()) {
            std::string currentName = j["currentPlaylist"].get<std::string>();
            setCurrentPlaylist(currentName);

            // Set current item
            if (j.contains("currentItemIndex") && j["currentItemIndex"].is_number()) {
                currentItemIndex = j["currentItemIndex"].get<size_t>();
                if (currentItemIndex >= currentPlaylist->getItemCount()) {
                    currentItemIndex = 0;
                }
            }
        }

        return true;
    } catch (const std::exception& e) {
        Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR,
                                                      "Error parsing playlist manager state: " + std::string(e.what()));
        return false;
    }
}

bool PlaylistManager::saveState(const std::string& filename) {
    std::string path;

    if (filename.empty()) {
        // Use default directory
        fs::path dirPath(defaultDirectory);
        fs::path filePath = dirPath / "playlist_state.json";
        path = filePath.string();

        // Create directory if it doesn't exist
        if (!fs::exists(dirPath)) {
            try {
                fs::create_directories(dirPath);
            } catch (const std::exception& e) {
                Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR,
                                                              "Failed to create playlist directory: " + std::string(e.what()));
                return false;
            }
        }
    } else {
        path = filename;
    }

    try {
        std::ofstream file(path);
        if (!file.is_open()) {
            Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR, "Failed to open file for writing: " + path);
            return false;
        }

        json j = toJson();
        file << j.dump(4);  // Pretty print with 4-space indentation
        return true;
    } catch (const std::exception& e) {
        Core::ErrorHandler::getInstance().handleError(Core::MediaPlayerException::DECODER_ERROR,
                                                      "Error saving playlist manager state: " + std::string(e.what()));
        return false;
    }
}

bool PlaylistManager::loadState(const std::string& filename) {
    std::string path;

    if (filename.empty()) {
        // Use default directory
        fs::path dirPath(defaultDirectory);
        fs::path filePath = dirPath / "playlist_state.json";
        path = filePath.string();
    } else {
        path = filename;
    }

    try {
        // Check if file exists
        if (!fs::exists(path)) {
            // Not an error, just create default playlist
            createPlaylist("Default");
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
                                                      "Error loading playlist manager state: " + std::string(e.what()));
        return false;
    }
}

}  // namespace Playlist
}  // namespace VideoPlayer
