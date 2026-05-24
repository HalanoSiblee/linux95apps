#ifndef UI_H
#define UI_H

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Menu_Bar.H>
#include <memory>
#include <string>
#include "player.h"

class MediaUI : public Fl_Window {
public:
    MediaUI(int w, int h, const char* title);
    ~MediaUI();
    
    void setPlayer(std::shared_ptr<Player> p) { player = p; }
    void loadPlaylist(const std::vector<std::string>& files);
    void handleDragDrop(const std::string& filepath);
    
    void updateProgress(double current, double total);
    void updatePlaylistDisplay();
    void updateStatus(const std::string& msg);
    
private:
    std::shared_ptr<Player> player;
    
    Fl_Box* videoBox = nullptr;
    Fl_Output* fileInfo = nullptr;
    Fl_Output* timeDisplay = nullptr;
    Fl_Slider* seekBar = nullptr;
    Fl_Button* playBtn = nullptr;
    Fl_Button* pauseBtn = nullptr;
    Fl_Button* stopBtn = nullptr;
    Fl_Button* nextBtn = nullptr;
    Fl_Button* prevBtn = nullptr;
    Fl_Output* playlistInfo = nullptr;
    
    bool seeking = false;
    
    static void playCallback(Fl_Widget*, void*);
    static void pauseCallback(Fl_Widget*, void*);
    static void stopCallback(Fl_Widget*, void*);
    static void nextCallback(Fl_Widget*, void*);
    static void prevCallback(Fl_Widget*, void*);
    static void seekCallback(Fl_Widget*, void*);
    static void openCallback(Fl_Widget*, void*);
    static int dragDropHandle(int event, Fl_Window* w);
};

#endif // UI_H
