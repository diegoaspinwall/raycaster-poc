# Simple, portable Makefile for SDL2 C project
# Tries pkg-config; falls back to sdl2-config if available

APP := raycaster
SRC := src/main.c
OBJ := $(SRC:.c=.o)
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Wno-unused-parameter

# Detect SDL2 flags
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs   sdl2 2>/dev/null)

ifeq ($(SDL_CFLAGS)$(SDL_LIBS),)
SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL_LIBS   := $(shell sdl2-config --libs   2>/dev/null)
endif

# macOS fallback if neither tool is available
ifeq ($(SDL_CFLAGS)$(SDL_LIBS),)
SDL_LIBS := -lSDL2
endif

all: $(APP)

$(APP): $(OBJ)
	$(CC) $(OBJ) $(SDL_LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

clean:
	rm -f $(APP) $(OBJ)

.PHONY: all clean
