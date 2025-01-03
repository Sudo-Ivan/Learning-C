CC = gcc
CFLAGS = `pkg-config --cflags gtk4 libadwaita-1 gstreamer-1.0 json-glib-1.0`
LIBS = `pkg-config --libs gtk4 libadwaita-1 gstreamer-1.0 json-glib-1.0`

TARGET = music-app
SRCS = gtk4-music-app/music-app.c gtk4-music-app/music-config.c gtk4-music-app/mpris.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean