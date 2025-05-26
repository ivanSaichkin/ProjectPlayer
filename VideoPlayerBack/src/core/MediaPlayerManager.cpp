#include "../../include/core/MediaPlayerManager.hpp"

#include <filesystem>

#include "../../include/core/ErrorHandler.hpp"

namespace fs = std::filesystem;
namespace VideoPlayer {
namespace Core {

MediaPlayerManager& MediaPlayerManager::getInstance() {
    static MediaPlayerManager instance;
    return instance;
}

MediaPlayerManager::MediaPlayerManager()
    : autoplay(true), rememberPosition(true), fullscreen(false), repeatMode(0), shuffleMode(false), muted(false), volumeBeforeMute(1.0f) {
    // Create media player
    mediaPlayer = std::make_unique<MediaPlayer>();

    // Set up internal callbacks
    mediaPlayer->setPlaybackStartCallback([this]() { onPlaybackStart(); });
    mediaPlayer->setPlaybackPauseCallback([this]() { onPlaybackPause(); });
    mediaPlayer->setPlaybackStopCallback([this]() { onPlaybackStop(); });
    mediaPlayer->setPlaybackEndCallback([this]() { onPlaybackEnd(); });
    mediaPlayer->setPositionChangeCallback([this](double pos) { onPositionChange(pos); });
    mediaPlayer->setErrorCallback([this](const MediaPlayerException& e) { onError(e); });
}

MediaPlayerManager::~MediaPlayerManager() {
    shutdown();
}

void MediaPlayerManager::loadSettings() {
    auto& config = Storage::ConfigManager::getInstance();

    // Load volume
    float volume = config.getValue<float>("volume", 1.0f);
    mediaPlayer->setVolume(volume);

    // Load autoplay setting
    autoplay = config.getValue<bool>("autoplay", true);

    // Load remember position setting
    rememberPosition = config.getValue<bool>("rememberPosition", true);

    // Load fullscreen setting
    fullscreen = config.getValue<bool>("fullscreen", false);

    // Load repeat mode
    repeatMode = config.getValue<int>("repeatMode", 0);

    // Load shuffle mode
    shuffleMode = config.getValue<bool>("shuffleMode", false);
}

bool MediaPlayerManager::initialize() {
    try {
        // Load settings
        loadSettings();

        // Initialize playlist manager
        auto& playlistManager = Playlist::PlaylistManager::getInstance();
        playlistManager.loadState();

        // Initialize history manager
        auto& historyManager = Storage::HistoryManager::getInstance();
        historyManager.loadHistory();

        return true;
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException(MediaPlayerException::INITIALIZATION_ERROR, e.what()));
        return false;
    }
}

void MediaPlayerManager::shutdown() {
    // Stop playback
    stop();

    // Save settings
    saveSettings();

    // Save playlist state
    Playlist::PlaylistManager::getInstance().saveState();

    // Save history
    Storage::HistoryManager::getInstance().saveHistory();
}

bool MediaPlayerManager::openFile(const std::string& filePath) {
    try {
        // Stop current playback
        stop();

        // Open the file
        if (!mediaPlayer->open(filePath)) {
            return false;
        }

        currentFilePath = filePath;

        // Add to history
        fs::path path(filePath);
        std::string title = path.filename().string();
        Storage::HistoryManager::getInstance().addEntry(filePath, title);

        // Check if we should restore position
        if (rememberPosition && Storage::HistoryManager::getInstance().hasEntry(filePath)) {
            double lastPosition = Storage::HistoryManager::getInstance().getEntry(filePath).lastPosition;
            if (lastPosition > 0 && lastPosition < mediaPlayer->getDuration() - 5.0) {
                mediaPlayer->seek(lastPosition);
            }
        }

        // Start playback if autoplay is enabled
        if (autoplay) {
            play();
        }

        return true;
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException(MediaPlayerException::FILE_OPEN_ERROR, e.what()));
        return false;
    }
}

bool MediaPlayerManager::openPlaylistItem(size_t index) {
    auto& playlistManager = Playlist::PlaylistManager::getInstance();

    if (!playlistManager.getCurrentPlaylist() || index >= playlistManager.getCurrentPlaylist()->getItemCount()) {
        return false;
    }

    // Set the current item
    playlistManager.setCurrentItem(index);

    // Get the item
    auto item = playlistManager.getCurrentItem();

    // Open the file
    return openFile(item.path);
}

void MediaPlayerManager::play() {
    if (!mediaPlayer->isOpen()) {
        // Try to open the last played file or first playlist item
        auto& historyManager = Storage::HistoryManager::getInstance();
        auto recentEntries = historyManager.getRecentEntries(1);

        if (!recentEntries.empty()) {
            openFile(recentEntries[0].filePath);
            return;
        }

        // No recent entries, try playlist
        auto& playlistManager = Playlist::PlaylistManager::getInstance();
        if (playlistManager.getCurrentPlaylist() && playlistManager.getCurrentPlaylist()->getItemCount() > 0) {
            openPlaylistItem(0);
            return;
        }

        // Nothing to play
        return;
    }

    mediaPlayer->play();
}

void MediaPlayerManager::pause() {
    mediaPlayer->pause();
}

void MediaPlayerManager::stop() {
    if (mediaPlayer->isPlaying() || mediaPlayer->isPaused()) {
        // Save position before stopping
        updateHistory();

        mediaPlayer->stop();
    }

    // Close the file
    mediaPlayer->close();
    currentFilePath.clear();
}

void MediaPlayerManager::seek(double position) {
    mediaPlayer->seek(position);
}

bool MediaPlayerManager::playNext() {
    auto& playlistManager = Playlist::PlaylistManager::getInstance();

    if (!playlistManager.hasNextItem()) {
        if (repeatMode == 2) {  // Repeat all
            // Go to first item
            playlistManager.setCurrentItem(0);
            auto item = playlistManager.getCurrentItem();
            return openFile(item.path);
        }
        return false;
    }

    // Save current position
    updateHistory();

    // Move to next item
    playlistManager.playNextItem();

    // Open the item
    auto item = playlistManager.getCurrentItem();
    return openFile(item.path);
}

bool MediaPlayerManager::playPrevious() {
    auto& playlistManager = Playlist::PlaylistManager::getInstance();

    // If we're more than 3 seconds into the track, seek to beginning instead
    if (mediaPlayer->getCurrentPosition() > 3.0) {
        mediaPlayer->seek(0.0);
        return true;
    }

    if (!playlistManager.hasPreviousItem()) {
        if (repeatMode == 2) {  // Repeat all
            // Go to last item
            size_t lastIndex = playlistManager.getCurrentPlaylist()->getItemCount() - 1;
            playlistManager.setCurrentItem(lastIndex);
            auto item = playlistManager.getCurrentItem();
            return openFile(item.path);
        }
        return false;
    }

    // Save current position
    updateHistory();

    // Move to previous item
    playlistManager.playPreviousItem();

    // Open the item
    auto item = playlistManager.getCurrentItem();
    return openFile(item.path);
}

void MediaPlayerManager::setVolume(float volume) {
    mediaPlayer->setVolume(volume);

    // If volume is set while muted, unmute
    if (muted && volume > 0) {
        muted = false;
    }

    // Save to config
    Storage::ConfigManager::getInstance().setValue("volume", volume);

    // Call user callback
    if (userVolumeChangeCallback) {
        userVolumeChangeCallback(volume);
    }
}

float MediaPlayerManager::getVolume() const {
    return mediaPlayer->getVolume();
}

void MediaPlayerManager::toggleMute() {
    if (muted) {
        // Unmute
        mediaPlayer->setVolume(volumeBeforeMute);
        muted = false;
    } else {
        // Mute
        volumeBeforeMute = mediaPlayer->getVolume();
        mediaPlayer->setVolume(0.0f);
        muted = true;
    }

    // Call user callback
    if (userVolumeChangeCallback) {
        userVolumeChangeCallback(mediaPlayer->getVolume());
    }
}

bool MediaPlayerManager::isMuted() const {
    return muted;
}

void MediaPlayerManager::createPlaylist(const std::string& name) {
    Playlist::PlaylistManager::getInstance().createPlaylist(name);
}

void MediaPlayerManager::loadPlaylist(const std::string& filePath) {
    Playlist::PlaylistManager::getInstance().loadPlaylist(filePath);
}

void MediaPlayerManager::saveCurrentPlaylist() {
    auto playlist = Playlist::PlaylistManager::getInstance().getCurrentPlaylist();
    if (playlist) {
        Playlist::PlaylistManager::getInstance().savePlaylist(playlist);
    }
}

void MediaPlayerManager::addToPlaylist(const std::string& filePath) {
    auto& playlistManager = Playlist::PlaylistManager::getInstance();
    auto playlist = playlistManager.getCurrentPlaylist();
    if (!playlist) {
        // Create default playlist if none exists
        playlist = playlistManager.createPlaylist("Default");
        playlistManager.setCurrentPlaylist(playlist);
    }

    // Add file to playlist
    fs::path path(filePath);
    playlist->addItem(filePath, path.filename().string());

    // Save playlist
    playlistManager.savePlaylist(playlist);
}

void MediaPlayerManager::removeFromPlaylist(size_t index) {
    auto& playlistManager = Playlist::PlaylistManager::getInstance();
    auto playlist = playlistManager.getCurrentPlaylist();
    if (!playlist) {
        return;
    }

    // Remove item
    playlist->removeItem(index);

    // Save playlist
    playlistManager.savePlaylist(playlist);
}

void MediaPlayerManager::clearPlaylist() {
    auto& playlistManager = Playlist::PlaylistManager::getInstance();
    auto playlist = playlistManager.getCurrentPlaylist();
    if (!playlist) {
        return;
    }

    // Clear playlist
    playlist->clear();

    // Save playlist
    playlistManager.savePlaylist(playlist);
}

std::vector<std::string> MediaPlayerManager::getPlaylistNames() const {
    return Playlist::PlaylistManager::getInstance().getPlaylistNames();
}

void MediaPlayerManager::setCurrentPlaylist(const std::string& name) {
    Playlist::PlaylistManager::getInstance().setCurrentPlaylist(name);
}

std::vector<Storage::FileInfo> MediaPlayerManager::listDirectory(const std::string& path) {
    return Storage::FileManager::getInstance().listDirectory(path);
}

std::vector<Storage::FileInfo> MediaPlayerManager::findMediaFiles(const std::string& directory, bool recursive) {
    return Storage::FileManager::getInstance().findMediaFiles(directory, recursive);
}

void MediaPlayerManager::setAutoplay(bool autoplay) {
    this->autoplay = autoplay;

    // Save to config
    Storage::ConfigManager::getInstance().setValue("autoplay", autoplay);
}

bool MediaPlayerManager::getAutoplay() const {
    return autoplay;
}

void MediaPlayerManager::setRememberPosition(bool remember) {
    this->rememberPosition = remember;

    // Save to config
    Storage::ConfigManager::getInstance().setValue("rememberPosition", remember);
}

bool MediaPlayerManager::getRememberPosition() const {
    return rememberPosition;
}

void MediaPlayerManager::setFullscreen(bool fullscreen) {
    this->fullscreen = fullscreen;

    // Save to config
    Storage::ConfigManager::getInstance().setValue("fullscreen", fullscreen);
}

bool MediaPlayerManager::isFullscreen() const {
    return fullscreen;
}

void MediaPlayerManager::setRepeatMode(int mode) {
    // Validate mode
    if (mode < 0 || mode > 2) {
        mode = 0;
    }

    this->repeatMode = mode;

    // Save to config
    Storage::ConfigManager::getInstance().setValue("repeatMode", repeatMode);
}

int MediaPlayerManager::getRepeatMode() const {
    return repeatMode;
}

void MediaPlayerManager::setShuffleMode(bool shuffle) {
    this->shuffleMode = shuffle;

    // Save to config
    Storage::ConfigManager::getInstance().setValue("shuffleMode", shuffleMode);
}

bool MediaPlayerManager::getShuffleMode() const {
    return shuffleMode;
}

bool MediaPlayerManager::isPlaying() const {
    return mediaPlayer->isPlaying();
}

bool MediaPlayerManager::isPaused() const {
    return mediaPlayer->isPaused();
}

bool MediaPlayerManager::isStopped() const {
    return !mediaPlayer->isPlaying() && !mediaPlayer->isPaused();
}

double MediaPlayerManager::getCurrentPosition() const {
    return mediaPlayer->getCurrentPosition();
}

double MediaPlayerManager::getDuration() const {
    return mediaPlayer->getDuration();
}

std::string MediaPlayerManager::getCurrentFilePath() const {
    return currentFilePath;
}

std::string MediaPlayerManager::getCurrentFileName() const {
    if (currentFilePath.empty()) {
        return "";
    }

    fs::path path(currentFilePath);
    return path.filename().string();
}

std::vector<Storage::HistoryEntry> MediaPlayerManager::getRecentEntries(size_t count) const {
    return Storage::HistoryManager::getInstance().getRecentEntries(count);
}

MediaPlayer& MediaPlayerManager::getMediaPlayer() {
    return *mediaPlayer;
}

Playlist::PlaylistManager& MediaPlayerManager::getPlaylistManager() {
    return Playlist::PlaylistManager::getInstance();
}

Storage::ConfigManager& MediaPlayerManager::getConfigManager() {
    return Storage::ConfigManager::getInstance();
}

Storage::HistoryManager& MediaPlayerManager::getHistoryManager() {
    return Storage::HistoryManager::getInstance();
}

Storage::FileManager& MediaPlayerManager::getFileManager() {
    return Storage::FileManager::getInstance();
}

void MediaPlayerManager::setPlaybackStartCallback(std::function<void()> callback) {
    userPlaybackStartCallback = std::move(callback);
}

void MediaPlayerManager::setPlaybackPauseCallback(std::function<void()> callback) {
    userPlaybackPauseCallback = std::move(callback);
}

void MediaPlayerManager::setPlaybackStopCallback(std::function<void()> callback) {
    userPlaybackStopCallback = std::move(callback);
}

void MediaPlayerManager::setPlaybackEndCallback(std::function<void()> callback) {
    userPlaybackEndCallback = std::move(callback);
}

void MediaPlayerManager::setPositionChangeCallback(std::function<void(double)> callback) {
    userPositionChangeCallback = std::move(callback);
}

void MediaPlayerManager::setVolumeChangeCallback(std::function<void(float)> callback) {
    userVolumeChangeCallback = std::move(callback);
}

void MediaPlayerManager::setFrameReadyCallback(std::function<void()> callback) {
    userFrameReadyCallback = std::move(callback);
    mediaPlayer->setFrameReadyCallback(std::move(callback));
}

void MediaPlayerManager::setErrorCallback(std::function<void(const MediaPlayerException&)> callback) {
    userErrorCallback = std::move(callback);
}

void MediaPlayerManager::onPlaybackStart() {
    // Update media info if needed
    if (!currentFilePath.empty()) {
        // Update duration in playlist
        auto& playlistManager = Playlist::PlaylistManager::getInstance();
        if (playlistManager.getCurrentPlaylist()) {
            size_t currentIndex = playlistManager.getCurrentItemIndex();
            playlistManager.getCurrentPlaylist()->updateItemDuration(currentIndex, mediaPlayer->getDuration());
        }
    }

    // Call user callback
    if (userPlaybackStartCallback) {
        userPlaybackStartCallback();
    }
}

void MediaPlayerManager::onPlaybackPause() {
    // Save position
    updateHistory();

    // Call user callback
    if (userPlaybackPauseCallback) {
        userPlaybackPauseCallback();
    }
}

void MediaPlayerManager::onPlaybackStop() {
    // Save position
    updateHistory();

    // Call user callback
    if (userPlaybackStopCallback) {
        userPlaybackStopCallback();
    }
}

void MediaPlayerManager::onPlaybackEnd() {
    // Handle end of playback based on repeat mode
    handlePlaybackEnd();

    // Call user callback
    if (userPlaybackEndCallback) {
        userPlaybackEndCallback();
    }
}

void MediaPlayerManager::onPositionChange(double position) {
    // Periodically save position (e.g., every 5 seconds)
    static double lastSavedPosition = 0.0;
    if (rememberPosition && !currentFilePath.empty() && std::abs(position - lastSavedPosition) > 5.0) {
        Storage::HistoryManager::getInstance().updateEntry(currentFilePath, position);
        lastSavedPosition = position;
    }

    // Call user callback
    if (userPositionChangeCallback) {
        userPositionChangeCallback(position);
    }
}

void MediaPlayerManager::onError(const MediaPlayerException& error) {
    // Log the error
    ErrorHandler::getInstance().handleError(error);

    // Call user callback
    if (userErrorCallback) {
        userErrorCallback(error);
    }
}

void MediaPlayerManager::saveSettings() {
    auto& config = Storage::ConfigManager::getInstance();

    // Save volume
    config.setValue("volume", mediaPlayer->getVolume());

    // Save autoplay setting
    config.setValue("autoplay", autoplay);

    // Save remember position setting
    config.setValue("rememberPosition", rememberPosition);

    // Save fullscreen setting
    config.setValue("fullscreen", fullscreen);

    // Save repeat mode
    config.setValue("repeatMode", repeatMode);

    // Save shuffle mode
    config.setValue("shuffleMode", shuffleMode);

    // Save config
    config.saveConfig();
}

void MediaPlayerManager::updateHistory() {
    if (rememberPosition && !currentFilePath.empty()) {
        // Update history
        Storage::HistoryManager::getInstance().updateEntry(currentFilePath, mediaPlayer->getCurrentPosition());

        // Update position in playlist
        auto& playlistManager = Playlist::PlaylistManager::getInstance();
        if (playlistManager.getCurrentPlaylist()) {
            size_t currentIndex = playlistManager.getCurrentItemIndex();
            playlistManager.getCurrentPlaylist()->updateItemPosition(currentIndex, mediaPlayer->getCurrentPosition());
        }
    }
}

void MediaPlayerManager::handlePlaybackEnd() {
    // Save final position
    updateHistory();

    // Determine what to do next based on repeat mode
    switch (repeatMode) {
        case 1:  // Repeat one
            // Restart the current file
            mediaPlayer->seek(0.0);
            mediaPlayer->play();
            break;

        case 2:  // Repeat all
            // Play next item or loop back to first
            if (!playNext()) {
                // If playNext fails, we're at the end of the playlist
                // Go to first item
                auto& playlistManager = Playlist::PlaylistManager::getInstance();
                if (playlistManager.getCurrentPlaylist() && playlistManager.getCurrentPlaylist()->getItemCount() > 0) {
                    playlistManager.setCurrentItem(0);
                    auto item = playlistManager.getCurrentItem();
                    openFile(item.path);
                }
            }
            break;

        default:  // No repeat
            // Just play next if available
            playNext();
            break;
    }
}

}  // namespace Core
}  // namespace VideoPlayer
