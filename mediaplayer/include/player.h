#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>
#include <mutex>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

class Player {
public:
    using PlaylistCallback = std::function<void(int, const std::string&)>;
    using StatusCallback = std::function<void(const std::string&)>;
    using ProgressCallback = std::function<void(double, double)>;

    Player();
    ~Player();

    void loadPlaylist(const std::vector<std::string>& files);
    void addToPlaylist(const std::string& filepath);
    void clearPlaylist();
    
    void play();
    void pause();
    void stop();
    void next();
    void previous();
    void seek(double seconds);
    
    void setCurrentTrack(int index);
    int getCurrentTrackIndex() const { return currentTrack; }
    const std::vector<std::string>& getPlaylist() const { return playlist; }
    
    bool isPlaying() const { return playing; }
    bool isPaused() const { return paused; }
    double getDuration() const { return duration; }
    double getCurrentTime() const { return currentTime; }
    std::string getFileName() const;
    
    void setPlaylistCallback(PlaylistCallback cb) { playlistCb = cb; }
    void setStatusCallback(StatusCallback cb) { statusCb = cb; }
    void setProgressCallback(ProgressCallback cb) { progressCb = cb; }

private:
    void playbackThread();
    bool openFile(const std::string& filepath);
    void closeFile();
    void updateProgress();
    void notifyStatus(const std::string& msg);
    void notifyPlaylist(int idx, const std::string& file);
    void notifyProgress();

    std::vector<std::string> playlist;
    int currentTrack = -1;
    
    AVFormatContext* formatCtx = nullptr;
    AVCodecContext* videoCodecCtx = nullptr;
    AVCodecContext* audioCodecCtx = nullptr;
    int videoStreamIdx = -1;
    int audioStreamIdx = -1;
    
    std::atomic<bool> playing{false};
    std::atomic<bool> paused{false};
    std::atomic<bool> shouldStop{false};
    std::atomic<double> currentTime{0.0};
    double duration = 0.0;
    
    std::unique_ptr<std::thread> playThread;
    std::mutex playerMutex;
    
    PlaylistCallback playlistCb;
    StatusCallback statusCb;
    ProgressCallback progressCb;
};

#endif // PLAYER_H
