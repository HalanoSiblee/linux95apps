#include "../include/mpris.h"
#include <iostream>

MPRIS::MPRIS(std::shared_ptr<Player> p) : player(p) {}

MPRIS::~MPRIS() {
    if (connection) {
        dbus_connection_unref(connection);
    }
}

bool MPRIS::init() {
    DBusError error;
    dbus_error_init(&error);
    
    connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    if (!connection) {
        std::cerr << "D-Bus connection failed: " << error.message << std::endl;
        dbus_error_free(&error);
        return false;
    }
    
    if (!registerBusName()) {
        std::cerr << "Failed to register MPRIS bus name" << std::endl;
        return false;
    }
    
    if (!registerInterfaces()) {
        std::cerr << "Failed to register MPRIS interfaces" << std::endl;
        return false;
    }
    
    std::cout << "MPRIS interface registered: " << busName << std::endl;
    return true;
}

bool MPRIS::registerBusName() {
    DBusError error;
    dbus_error_init(&error);
    
    int result = dbus_bus_request_name(
        connection,
        busName.c_str(),
        DBUS_NAME_FLAG_REPLACE_EXISTING,
        &error
    );
    
    if (dbus_error_is_set(&error)) {
        std::cerr << "D-Bus error: " << error.message << std::endl;
        dbus_error_free(&error);
        return false;
    }
    
    return result == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER;
}

bool MPRIS::registerInterfaces() {
    DBusObjectPathVTable vtable;
    vtable.unregister_function = nullptr;
    vtable.message_function = messageHandler;
    
    if (!dbus_connection_register_object_path(
        connection,
        "/org/mpris/MediaPlayer2",
        &vtable,
        this
    )) {
        return false;
    }
    
    dbus_bus_add_match(
        connection,
        "type='method_call',interface='org.mpris.MediaPlayer2.Player'",
        nullptr
    );
    
    return true;
}

void MPRIS::update() {
    if (connection) {
        dbus_connection_read_write(connection, 0);
        
        DBusMessage* msg = dbus_connection_pop_message(connection);
        while (msg) {
            dbus_message_unref(msg);
            msg = dbus_connection_pop_message(connection);
        }
    }
}

void MPRIS::notifyPropertiesChanged() {
    if (!connection) return;
    
    DBusMessage* msg = dbus_message_new_signal(
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged"
    );
    
    if (msg) {
        dbus_connection_send(connection, msg, nullptr);
        dbus_message_unref(msg);
    }
}

DBusHandlerResult MPRIS::messageHandler(
    DBusConnection* conn,
    DBusMessage* msg,
    void* userData
) {
    MPRIS* mpris = static_cast<MPRIS*>(userData);
    
    const char* method = dbus_message_get_member(msg);
    
    if (!method) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    
    if (strcmp(method, "Play") == 0) {
        mpris->player->play();
    } else if (strcmp(method, "Pause") == 0) {
        mpris->player->pause();
    } else if (strcmp(method, "Stop") == 0) {
        mpris->player->stop();
    } else if (strcmp(method, "Next") == 0) {
        mpris->player->next();
    } else if (strcmp(method, "Previous") == 0) {
        mpris->player->previous();
    } else if (strcmp(method, "Seek") == 0) {
        int64_t offset;
        if (dbus_message_get_args(
            msg,
            nullptr,
            DBUS_TYPE_INT64,
            &offset,
            DBUS_TYPE_INVALID
        )) {
            double seconds = offset / 1000000.0;
            mpris->player->seek(mpris->player->getCurrentTime() + seconds);
        }
    } else if (strcmp(method, "SetPosition") == 0) {
        const char* trackId;
        int64_t position;
        if (dbus_message_get_args(
            msg,
            nullptr,
            DBUS_TYPE_OBJECT_PATH,
            &trackId,
            DBUS_TYPE_INT64,
            &position,
            DBUS_TYPE_INVALID
        )) {
            double seconds = position / 1000000.0;
            mpris->player->seek(seconds);
        }
    }
    
    DBusMessage* reply = dbus_message_new_method_return(msg);
    if (reply) {
        dbus_connection_send(conn, reply, nullptr);
        dbus_message_unref(reply);
    }
    
    return DBUS_HANDLER_RESULT_HANDLED;
}

std::string MPRIS::getPlaybackStatus() const {
    if (player->isPlaying()) {
        return player->isPaused() ? "Paused" : "Playing";
    }
    return "Stopped";
}
