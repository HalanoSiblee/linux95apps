#include "../include/ui.h"
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/fl_ask.H>
#include <sstream>
#include <iomanip>

MediaUI::MediaUI(int w, int h, const char* title)
    : Fl_Window(w, h, title) {
    
    Fl_Menu_Bar* menu = new Fl_Menu_Bar(0, 0, w, 25);
    menu->add("&File/&Open...", FL_CTRL + 'o', openCallback, this);
    menu->add("&File/E&xit", FL_CTRL + 'q', [](Fl_Widget*, void* w) {
        ((Fl_Window*)w)->hide();
    }, this);
    
    videoBox = new Fl_Box(10, 35, w - 20, 400);
    videoBox->box(FL_FLAT_BOX);
    videoBox->color(FL_BLACK);
    
    fileInfo = new Fl_Output(10, 445, w - 20, 25);
    fileInfo->value("No file loaded");
    
    timeDisplay = new Fl_Output(10, 475, w - 20, 25);
    timeDisplay->value("00:00 / 00:00");
    
    seekBar = new Fl_Slider(10, 505, w - 20, 20);
    seekBar->type(FL_HOR_SLIDER);
    seekBar->bounds(0.0, 100.0);
    seekBar->value(0.0);
    seekBar->callback(seekCallback, this);
    
    int btn_width = 90;
    int btn_y = 535;
    
    playBtn = new Fl_Button(10, btn_y, btn_width, 30, "Play");
    playBtn->callback(playCallback, this);
    
    pauseBtn = new Fl_Button(105, btn_y, btn_width, 30, "Pause");
    pauseBtn->callback(pauseCallback, this);
    
    stopBtn = new Fl_Button(200, btn_y, btn_width, 30, "Stop");
    stopBtn->callback(stopCallback, this);
    
    prevBtn = new Fl_Button(295, btn_y, btn_width, 30, "<< Prev");
    prevBtn->callback(prevCallback, this);
    
    nextBtn = new Fl_Button(390, btn_y, btn_width, 30, "Next >>");
    nextBtn->callback(nextCallback, this);
    
    playlistInfo = new Fl_Output(10, 575, w - 20, 105);
    playlistInfo->value("Playlist empty. Drag & drop files or use File > Open");
    
    end();
}

MediaUI::~MediaUI() = default;

void MediaUI::loadPlaylist(const std::vector<std::string>& files) {
    if (player) {
        player->loadPlaylist(files);
    }
    updatePlaylistDisplay();
}

void MediaUI::handleDragDrop(const std::string& filepath) {
    if (player) {
        player->addToPlaylist(filepath);
        updatePlaylistDisplay();
    }
}

void MediaUI::updateProgress(double current, double total) {
    if (total > 0) {
        seekBar->value((current / total) * 100.0);
    }
    
    auto formatTime = [](double seconds) -> std::string {
        int mins = (int)seconds / 60;
        int secs = (int)seconds % 60;
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << mins << ":"
            << std::setfill('0') << std::setw(2) << secs;
        return oss.str();
    };
    
    std::string timeStr = formatTime(current) + " / " + formatTime(total);
    timeDisplay->value(timeStr.c_str());
}

void MediaUI::updatePlaylistDisplay() {
    if (!player) return;
    
    const auto& playlist = player->getPlaylist();
    std::string info = "Playlist (" + std::to_string(playlist.size()) + "):\n";
    
    int current = player->getCurrentTrackIndex();
    for (size_t i = 0; i < playlist.size() && i < 5; ++i) {
        std::string name = playlist[i];
        size_t pos = name.find_last_of("/\\");
        if (pos != std::string::npos) name = name.substr(pos + 1);
        
        if ((int)i == current) {
            info += "► " + name + "\n";
        } else {
            info += "  " + name + "\n";
        }
    }
    
    if (playlist.size() > 5) {
        info += "  ... and " + std::to_string(playlist.size() - 5) + " more";
    }
    
    playlistInfo->value(info.c_str());
}

void MediaUI::updateStatus(const std::string& msg) {
    fileInfo->value(msg.c_str());
}

void MediaUI::playCallback(Fl_Widget*, void* data) {
    MediaUI* ui = static_cast<MediaUI*>(data);
    if (ui->player) ui->player->play();
}

void MediaUI::pauseCallback(Fl_Widget*, void* data) {
    MediaUI* ui = static_cast<MediaUI*>(data);
    if (ui->player) ui->player->pause();
}

void MediaUI::stopCallback(Fl_Widget*, void* data) {
    MediaUI* ui = static_cast<MediaUI*>(data);
    if (ui->player) ui->player->stop();
}

void MediaUI::nextCallback(Fl_Widget*, void* data) {
    MediaUI* ui = static_cast<MediaUI*>(data);
    if (ui->player) ui->player->next();
}

void MediaUI::prevCallback(Fl_Widget*, void* data) {
    MediaUI* ui = static_cast<MediaUI*>(data);
    if (ui->player) ui->player->previous();
}

void MediaUI::seekCallback(Fl_Widget* widget, void* data) {
    MediaUI* ui = static_cast<MediaUI*>(data);
    if (ui->player) {
        Fl_Slider* slider = static_cast<Fl_Slider*>(widget);
        double percent = slider->value() / 100.0;
        double seekPos = percent * ui->player->getDuration();
        ui->player->seek(seekPos);
    }
}

void MediaUI::openCallback(Fl_Widget*, void* data) {
    MediaUI* ui = static_cast<MediaUI*>(data);
    const char* filename = fl_file_chooser(
        "Open Media File",
        "Media (*.{mp4,mkv,avi,mov,flv,webm,mp3,aac,wav,flac})",
        nullptr
    );
    
    if (filename) {
        if (ui->player) {
            ui->player->addToPlaylist(filename);
            ui->player->setCurrentTrack(ui->player->getPlaylist().size() - 1);
        }
    }
}
