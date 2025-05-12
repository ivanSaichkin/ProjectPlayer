#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../core/MediaPlayer.hpp"
#include "../playlist/PlaylistManager.hpp"
#include "../storage/ConfigManager.hpp"
#include "../storage/FileManager.hpp"
#include "../storage/HistoryManager.hpp"

namespace VideoPlayer {
namespace Core {

// Forward declarations
class MediaPlayerException;

class MediaPlayerManager {
 public:
    // Singleton pattern
    static MediaPlayerManager& getInstance();

    // Initialization and cleanup
    bool initialize();
    void shutdown();

    // Media playback control
    bool openFile(const std::string& filePath);
    bool openPlaylistItem(size_t index);
    void play();
    void pause();
    void stop();
    void seek(double position);
    bool playNext();
    bool playPrevious();

    // Volume control
    void setVolume(float volume);
    float getVolume() const;
    void toggleMute();
    bool isMuted() const;

    // Playlist management
    void createPlaylist(const std::string& name);
    void loadPlaylist(const std::string& filePath);
    void saveCurrentPlaylist();
    void addToPlaylist(const std::string& filePath);
    void removeFromPlaylist(size_t index);
    void clearPlaylist();
    std::vector<std::string> getPlaylistNames() const;
    void setCurrentPlaylist(const std::string& name);

    // File discovery
    std::vector<Storage::FileInfo> listDirectory(const std::string& path);
    std::vector<Storage::FileInfo> findMediaFiles(const std::string& directory, bool recursive = false);

    // Settings
    void setAutoplay(bool autoplay);
    bool getAutoplay() const;
    void setRememberPosition(bool remember);
    bool getRememberPosition() const;
    void setFullscreen(bool fullscreen);
    bool isFullscreen() const;
    void setRepeatMode(int mode);  // 0: no repeat, 1: repeat one, 2: repeat all
    int getRepeatMode() const;
    void setShuffleMode(bool shuffle);
    bool getShuffleMode() const;

    // Status information
    bool isPlaying() const;
    bool isPaused() const;
    bool isStopped() const;
    double getCurrentPosition() const;
    double getDuration() const;
    std::string getCurrentFilePath() const;
    std::string getCurrentFileName() const;

    // Recent history
    std::vector<Storage::HistoryEntry> getRecentEntries(size_t count = 10) const;

    // Access to underlying components
    MediaPlayer& getMediaPlayer();
    Playlist::PlaylistManager& getPlaylistManager();
    Storage::ConfigManager& getConfigManager();
    Storage::HistoryManager& getHistoryManager();
    Storage::FileManager& getFileManager();

    // Event callbacks
    void setPlaybackStartCallback(std::function<void()> callback);
    void setPlaybackPauseCallback(std::function<void()> callback);
    void setPlaybackStopCallback(std::function<void()> callback);
    void setPlaybackEndCallback(std::function<void()> callback);
    void setPositionChangeCallback(std::function<void(double)> callback);
    void setVolumeChangeCallback(std::function<void(float)> callback);
    void setFrameReadyCallback(std::function<void()> callback);
    void setErrorCallback(std::function<void(const MediaPlayerException&)> callback);

 private:
    MediaPlayerManager();
    ~MediaPlayerManager();
    MediaPlayerManager(const MediaPlayerManager&) = delete;
    MediaPlayerManager& operator=(const MediaPlayerManager&) = delete;

    // Internal components
    std::unique_ptr<MediaPlayer> mediaPlayer;

    // Internal callbacks
    void onPlaybackStart();
    void onPlaybackPause();
    void onPlaybackStop();
    void onPlaybackEnd();
    void onPositionChange(double position);
    void onError(const MediaPlayerException& error);

    // User callbacks
    std::function<void()> userPlaybackStartCallback;
    std::function<void()> userPlaybackPauseCallback;
    std::function<void()> userPlaybackStopCallback;
    std::function<void()> userPlaybackEndCallback;
    std::function<void(double)> userPositionChangeCallback;
    std::function<void(float)> userVolumeChangeCallback;
    std::function<void()> userFrameReadyCallback;
    std::function<void(const MediaPlayerException&)> userErrorCallback;

    // Settings
    bool autoplay;
    bool rememberPosition;
    bool fullscreen;
    int repeatMode;
    bool shuffleMode;
    bool muted;
    float volumeBeforeMute;

    // Current media info
    std::string currentFilePath;

    // Load settings from config
    void loadSettings();
    // Save settings to config
    void saveSettings();
    // Update history with current position
    void updateHistory();
    // Handle end of playback
    void handlePlaybackEnd();
};

}  // namespace Core
}  // namespace VideoPlayer
