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

CXXFLAGS += $(FLTK_CXXFLAGS) $(FFMPEG_CXXFLAGS) $(DBUS_CXXFLAGS)
LDFLAGS += $(FLTK_LDFLAGS) $(FFMPEG_LDFLAGS) $(DBUS_LDFLAGS)

TARGET := mediaplayer
SOURCES := src/main.cpp src/player.cpp src/ui.cpp src/mpris.cpp
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
