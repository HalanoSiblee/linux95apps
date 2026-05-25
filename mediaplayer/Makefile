CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -fPIC
LDFLAGS := -lstdc++ -lm

# FLTK 1.4
FLTK_CXXFLAGS := $(shell fltk-config --cxxflags)
FLTK_LDFLAGS := $(shell fltk-config --ldflags)

# FFmpeg
FFMPEG_CXXFLAGS := $(shell pkg-config --cflags libavformat libavcodec libavutil libswscale libswresample)
FFMPEG_LDFLAGS := $(shell pkg-config --libs libavformat libavcodec libavutil libswscale libswresample)

# D-Bus
DBUS_CXXFLAGS := $(shell pkg-config --cflags dbus-1)
DBUS_LDFLAGS := $(shell pkg-config --libs dbus-1)

# PipeWire
PIPEWIRE_CXXFLAGS := $(shell pkg-config --cflags libpipewire-0.3 libspa-0.2 2>/dev/null || echo "")
PIPEWIRE_LDFLAGS := $(shell pkg-config --libs libpipewire-0.3 libspa-0.2 2>/dev/null || echo "")

# PulseAudio fallback
ifeq ($(PIPEWIRE_LDFLAGS),)
    PULSEAUDIO_CXXFLAGS := $(shell pkg-config --cflags libpulse-simple)
    PULSEAUDIO_LDFLAGS := $(shell pkg-config --libs libpulse-simple)
    AUDIO_BACKEND := pulseaudio
else
    PULSEAUDIO_CXXFLAGS :=
    PULSEAUDIO_LDFLAGS :=
    AUDIO_BACKEND := pipewire
endif

CXXFLAGS += $(FLTK_CXXFLAGS) $(FFMPEG_CXXFLAGS) $(DBUS_CXXFLAGS) $(PIPEWIRE_CXXFLAGS) $(PULSEAUDIO_CXXFLAGS)
LDFLAGS += $(FLTK_LDFLAGS) $(FFMPEG_LDFLAGS) $(DBUS_LDFLAGS) $(PIPEWIRE_LDFLAGS) $(PULSEAUDIO_LDFLAGS)

TARGET := mediaplayer
SOURCES := src/main.cpp src/player.cpp src/ui.cpp src/mpris.cpp src/audio.cpp
OBJECTS := $(SOURCES:.cpp=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)
