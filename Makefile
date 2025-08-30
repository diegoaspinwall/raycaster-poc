# Portable SDL2 Makefile using pkg-config first, then fallbacks.

APP := raycaster.out
SRC := src/main.c
OBJ := $(SRC:.c=.o)
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Wno-unused-parameter

PKG_CONFIG ?= pkg-config
SDL_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2 2>/dev/null)
SDL_LIBS   := $(shell $(PKG_CONFIG) --libs   sdl2 2>/dev/null)

# Fallback to sdl2-config if pkg-config not available
ifeq ($(SDL_CFLAGS)$(SDL_LIBS),)
SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL_LIBS   := $(shell sdl2-config --libs   2>/dev/null)
endif

# Homebrew path fallback (Apple Silicon or Intel Homebrew prefix)
ifeq ($(SDL_CFLAGS)$(SDL_LIBS),)
ifneq (,$(wildcard /opt/homebrew/include/SDL2/SDL.h))
SDL_CFLAGS := -I/opt/homebrew/include/SDL2
SDL_LIBS   := -L/opt/homebrew/lib -lSDL2
else ifneq (,$(wildcard /usr/local/include/SDL2/SDL.h))
SDL_CFLAGS := -I/usr/local/include/SDL2
SDL_LIBS   := -L/usr/local/lib -lSDL2
endif
endif

# Framework fallback for manual .framework installs on macOS
ifeq ($(SDL_CFLAGS)$(SDL_LIBS),)
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
SDL_LIBS := -F/Library/Frameworks -framework SDL2
endif
endif

all: $(APP)

$(APP): $(OBJ)
	$(CC) $(OBJ) $(SDL_LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

clean:
	rm -f $(APP) $(OBJ)

.PHONY: all clean
