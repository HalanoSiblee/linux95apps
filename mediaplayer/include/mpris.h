#ifndef MPRIS_H
#define MPRIS_H

#include <string>
#include <memory>
#include <dbus/dbus.h>
#include "player.h"

class MPRIS {
public:
    MPRIS(std::shared_ptr<Player> player);
    ~MPRIS();
    
    bool init();
    void update();
    void notifyPropertiesChanged();
    
private:
    std::shared_ptr<Player> player;
    DBusConnection* connection = nullptr;
    std::string busName = "org.mpris.MediaPlayer2.ffmpeg_fltk_player";
    
    bool registerBusName();
    bool registerInterfaces();
    
    static DBusHandlerResult messageHandler(
        DBusConnection* conn,
        DBusMessage* msg,
        void* userData
    );
    
    std::string getPlaybackStatus() const;
};

#endif // MPRIS_H
