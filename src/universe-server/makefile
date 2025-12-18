#TODO: remove unused flags

COMPILER = gcc
CFLAGS = -Wall -Wextra -g -Werror -pedantic
MATHFLAGS = -lm
CONFIGFLAGS = -lconfig
SDLFLAGS = -lSDL2 -lSDL2_image -lSDL2_gfx -lSDL2_ttf
ZMQFLAGS = -lzmq
PROTOCFLAGS = -lprotobuf-c

TARGET = universe-server
SRCS = universe-server.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(COMPILER) $(CFLAGS) -o $(TARGET) $(SRCS) $(CONFIGFLAGS) $(SDLFLAGS) $(ZMQFLAGS) $(PROTOCFLAGS) $(MATHFLAGS)

clean:
	rm -f $(TARGET)