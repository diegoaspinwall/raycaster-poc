# SDL2 Raycaster POC (C)

A tiny, self‑contained proof‑of‑concept for a DOOM/Wolf3D‑style 2.5D raycaster written in C using SDL2. No external assets required.

---

## Project layout
```
raycaster-poc/
├─ Makefile
├─ README.md
└─ src/
   └─ main.c
```

---

## Build & run

### macOS (Homebrew)
```bash
brew install sdl2
make
./raycaster
```

### Linux (Debian/Ubuntu)
```bash
sudo apt update && sudo apt install -y libsdl2-dev build-essential
make
./raycaster
```

**Controls**: `W/S` forward/back, `A/D` strafe, `←/→` turn, close window to quit.

## What you can add next
- **Textures:** bring in `SDL2_image`, store a few 64×64 wall textures, compute `tex_x` from the wall hit, and step `tex_y` along the column.
- **Sprites:** keep `(sx, sy)` for billboards, project using the same `perpDist`; clip against the `depth[]` buffer.
- **Doors & triggers:** reserve specific wall IDs for interactive tiles and toggle them.
- **Sector engine:** evolve beyond a grid with vertices/linedefs/sectors and portal rendering.

