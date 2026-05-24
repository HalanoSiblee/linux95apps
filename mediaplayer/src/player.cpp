#include "../include/player.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cmath>

Player::Player() {
    av_log_set_level(AV_LOG_ERROR);
}

Player::~Player() {
    stop();
    closeFile();
}

void Player::loadPlaylist(const std::vector<std::string>& files) {
    std::lock_guard<std::mutex> lock(playerMutex);
    playlist = files;
    currentTrack = -1;
    notifyStatus("Loaded " + std::to_string(files.size()) + " files");
}

void Player::addToPlaylist(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(playerMutex);
    playlist.push_back(filepath);
    notifyPlaylist(playlist.size() - 1, filepath);
}

void Player::clearPlaylist() {
    std::lock_guard<std::mutex> lock(playerMutex);
    stop();
    playlist.clear();
    currentTrack = -1;
}

void Player::play() {
    if (playlist.empty()) return;
    
    if (currentTrack < 0) {
        setCurrentTrack(0);
    } else if (paused) {
        paused = false;
        return;
    }
    
    if (playThread && playThread->joinable()) {
        return;
    }
    
    playing = true;
    shouldStop = false;
    playThread = std::make_unique<std::thread>([this]() { playbackThread(); });
}

void Player::pause() {
    paused = !paused;
    notifyStatus(paused ? "Paused" : "Resumed");
}

void Player::stop() {
    playing = false;
    shouldStop = true;
    if (playThread && playThread->joinable()) {
        playThread->join();
    }
    playThread.reset();
    closeFile();
    currentTime = 0.0;
    notifyStatus("Stopped");
}

void Player::next() {
    if (currentTrack + 1 < (int)playlist.size()) {
        setCurrentTrack(currentTrack + 1);
        if (playing) {
            stop();
            play();
        }
    }
}

void Player::previous() {
    if (currentTrack > 0) {
        setCurrentTrack(currentTrack - 1);
        if (playing) {
            stop();
            play();
        }
    }
}

void Player::seek(double seconds) {
    if (formatCtx && seconds >= 0 && seconds <= duration) {
        int64_t timestamp = static_cast<int64_t>(seconds * AV_TIME_BASE);
        av_seek_frame(formatCtx, -1, timestamp, AVSEEK_FLAG_BACKWARD);
        currentTime = seconds;
        notifyProgress();
    }
}

void Player::setCurrentTrack(int index) {
    if (index >= 0 && index < (int)playlist.size()) {
        currentTrack = index;
        if (openFile(playlist[index])) {
            notifyPlaylist(index, playlist[index]);
            notifyStatus("Opened: " + getFileName());
        }
    }
}

bool Player::openFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(playerMutex);
    closeFile();
    
    formatCtx = avformat_alloc_context();
    if (!formatCtx) return false;
    
    if (avformat_open_input(&formatCtx, filepath.c_str(), nullptr, nullptr) != 0) {
        avformat_free_context(formatCtx);
        formatCtx = nullptr;
        return false;
    }
    
    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        avformat_close_input(&formatCtx);
        formatCtx = nullptr;
        return false;
    }
    
    videoStreamIdx = -1;
    audioStreamIdx = -1;
    
    for (unsigned int i = 0; i < formatCtx->nb_streams; ++i) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStreamIdx < 0) {
            videoStreamIdx = i;
        }
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioStreamIdx < 0) {
            audioStreamIdx = i;
        }
    }
    
    if (videoStreamIdx >= 0) {
        AVCodecContext* ctx = avcodec_alloc_context3(nullptr);
        if (ctx) {
            avcodec_parameters_to_context(ctx, formatCtx->streams[videoStreamIdx]->codecpar);
            const AVCodec* codec = avcodec_find_decoder(ctx->codec_id);
            if (codec && avcodec_open2(ctx, codec, nullptr) == 0) {
                videoCodecCtx = ctx;
            } else {
                avcodec_free_context(&ctx);
            }
        }
    }
    
    if (audioStreamIdx >= 0) {
        AVCodecContext* ctx = avcodec_alloc_context3(nullptr);
        if (ctx) {
            avcodec_parameters_to_context(ctx, formatCtx->streams[audioStreamIdx]->codecpar);
            const AVCodec* codec = avcodec_find_decoder(ctx->codec_id);
            if (codec && avcodec_open2(ctx, codec, nullptr) == 0) {
                audioCodecCtx = ctx;
            } else {
                avcodec_free_context(&ctx);
            }
        }
    }
    
    if (formatCtx->duration != AV_NOPTS_VALUE) {
        duration = formatCtx->duration / (double)AV_TIME_BASE;
    }
    
    currentTime = 0.0;
    return true;
}

void Player::closeFile() {
    if (videoCodecCtx) {
        avcodec_free_context(&videoCodecCtx);
        videoCodecCtx = nullptr;
    }
    if (audioCodecCtx) {
        avcodec_free_context(&audioCodecCtx);
        audioCodecCtx = nullptr;
    }
    if (formatCtx) {
        avformat_close_input(&formatCtx);
        formatCtx = nullptr;
    }
    duration = 0.0;
    currentTime = 0.0;
    videoStreamIdx = -1;
    audioStreamIdx = -1;
}

void Player::playbackThread() {
    AVPacket* packet = av_packet_alloc();
    if (!packet) return;
    
    int64_t startTime = av_gettime();
    
    while (playing && !shouldStop) {
        if (paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        {
            std::lock_guard<std::mutex> lock(playerMutex);
            if (!formatCtx) break;
            
            int ret = av_read_frame(formatCtx, packet);
            if (ret < 0) {
                av_packet_free(&packet);
                next();
                break;
            }
            
            if (packet->stream_index == videoStreamIdx && videoCodecCtx) {
                avcodec_send_packet(videoCodecCtx, packet);
                AVFrame* frame = av_frame_alloc();
                if (avcodec_receive_frame(videoCodecCtx, frame) == 0) {
                    if (frame->pts != AV_NOPTS_VALUE) {
                        currentTime = frame->pts * av_q2d(formatCtx->streams[videoStreamIdx]->time_base);
                    }
                    av_frame_free(&frame);
                }
            } else if (packet->stream_index == audioStreamIdx && audioCodecCtx) {
                avcodec_send_packet(audioCodecCtx, packet);
                AVFrame* frame = av_frame_alloc();
                if (avcodec_receive_frame(audioCodecCtx, frame) == 0) {
                    if (frame->pts != AV_NOPTS_VALUE) {
                        currentTime = frame->pts * av_q2d(formatCtx->streams[audioStreamIdx]->time_base);
                    }
                    av_frame_free(&frame);
                }
            }
            
            av_packet_unref(packet);
        }
        
        notifyProgress();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    av_packet_free(&packet);
}

std::string Player::getFileName() const {
    if (currentTrack >= 0 && currentTrack < (int)playlist.size()) {
        std::string path = playlist[currentTrack];
        size_t pos = path.find_last_of("/\\");
        return (pos != std::string::npos) ? path.substr(pos + 1) : path;
    }
    return "No file";
}

void Player::notifyStatus(const std::string& msg) {
    if (statusCb) statusCb(msg);
}

void Player::notifyPlaylist(int idx, const std::string& file) {
    if (playlistCb) playlistCb(idx, file);
}

void Player::notifyProgress() {
    if (progressCb) progressCb(currentTime, duration);
}
