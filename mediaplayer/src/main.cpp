#include <FL/Fl.H>
#include "../include/ui.h"
#include "../include/player.h"
#include "../include/mpris.h"
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
    auto player = std::make_shared<Player>();
    
    MediaUI* window = new MediaUI(1000, 700, "FFmpeg Media Player");
    window->setPlayer(player);
    
    auto mpris = std::make_unique<MPRIS>(player);
    if (!mpris->init()) {
        std::cerr << "Warning: MPRIS initialization failed" << std::endl;
    }
    
    player->setPlaylistCallback([window](int idx, const std::string& file) {
        window->updatePlaylistDisplay();
    });
    
    player->setStatusCallback([window](const std::string& msg) {
        window->updateStatus(msg);
    });
    
    player->setProgressCallback([window](double current, double total) {
        window->updateProgress(current, total);
    });
    
    if (argc > 1) {
        std::vector<std::string> files;
        for (int i = 1; i < argc; ++i) {
            files.push_back(argv[i]);
        }
        player->loadPlaylist(files);
        window->loadPlaylist(files);
    }
    
    window->show();
    
    while (Fl::wait() > 0) {
        if (mpris) {
            mpris->update();
        }
    }
    
    return 0;
}
