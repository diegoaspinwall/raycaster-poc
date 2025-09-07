# SDL2 Raytrace (single-triangle) — focused build

RT_APP := rt
RT_SRC := src/main.c src/rays.c
RT_OBJ := $(RT_SRC:.c=.o)

CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Wno-unused-parameter

# Homebrew SDL2 paths (Apple Silicon). Change to /usr/local on Intel Macs if needed.
SDL_CFLAGS := -I/opt/homebrew/include
SDL_LIBS   := -L/opt/homebrew/lib -lSDL2

all: $(RT_APP)

$(RT_APP): $(RT_OBJ)
	$(CC) $(RT_OBJ) $(SDL_LIBS) -o $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

clean:
	rm -f $(RT_APP) $(RT_OBJ)

print-sdl:
	@echo SDL_CFLAGS=$(SDL_CFLAGS)
	@echo SDL_LIBS=$(SDL_LIBS)

.PHONY: all clean print-sdl
