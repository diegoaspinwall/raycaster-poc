// build (mac/linux): clang src/main.c -lSDL2 -O2 -std=c11 -o raycaster
// portable build: use the provided Makefile
#include <SDL2/SDL.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#define W  960
#define H  540
#define MAPW 24
#define MAPH 24

// 0 = empty, 1..n = wall ids
static int worldMap[MAPH][MAPW] = {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

static void draw_rect(SDL_Renderer* r, int x, int y, int w, int h, SDL_Color c) {
  SDL_Rect rr = {x,y,w,h};
  SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
  SDL_RenderFillRect(r, &rr);
}

int main(void) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) return 1;
  SDL_Window* win = SDL_CreateWindow("Raycaster POC", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_SHOWN);
  SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!ren) { SDL_Quit(); return 1; }

  double px = 12.0, py = 12.0;   // player position
  double ang = 0.0;              // radians
  const double FOV = M_PI/3.0;   // 60 deg
  const double move = 3.0;       // units / s
  const double turn = 2.5;       // rad / s
  double depth[W];

  uint64_t last = SDL_GetPerformanceCounter();
  bool run = true;
  while (run) {
    // --- timestep ---
    uint64_t now = SDL_GetPerformanceCounter();
    double dt = (double)(now - last) / SDL_GetPerformanceFrequency();
    if (dt > 0.05) dt = 0.05; // clamp
    last = now;

    // --- input ---
    SDL_Event e; while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) run = false;
    }
    const Uint8* k = SDL_GetKeyboardState(NULL);
    if (k[SDL_SCANCODE_LEFT])  ang -= turn * dt;
    if (k[SDL_SCANCODE_RIGHT]) ang += turn * dt;

    double dx = cos(ang), dy = sin(ang);
    double mx = 0, my = 0;
    if (k[SDL_SCANCODE_W]) { mx += dx * move * dt; my += dy * move * dt; }
    if (k[SDL_SCANCODE_S]) { mx -= dx * move * dt; my -= dy * move * dt; }
    if (k[SDL_SCANCODE_A]) { mx += -dy * move * dt; my +=  dx * move * dt; }
    if (k[SDL_SCANCODE_D]) { mx +=  dy * move * dt; my += -dx * move * dt; }

    // collision (circle radius r = 0.2)
    double nx = px + mx, ny = py + my, r = 0.2;
    if (worldMap[(int)py][(int)(nx + (mx>0? r : -r))] == 0) px = nx;
    if (worldMap[(int)(ny + (my>0? r : -r))][(int)px] == 0) py = ny;

    // --- render background ---
    SDL_SetRenderDrawColor(ren, 20,20,25,255); SDL_RenderClear(ren);
    draw_rect(ren, 0, 0, W, H/2, (SDL_Color){60,60,90,255});   // sky
    draw_rect(ren, 0, H/2, W, H/2, (SDL_Color){35,35,35,255}); // ground

    // --- raycast walls ---
    for (int x = 0; x < W; ++x) {
      double cameraX = (2.0 * x / (double)W - 1.0); // -1..1
      double rayAng = ang + atan(cameraX * tan(FOV/2.0));
      double rx = cos(rayAng), ry = sin(rayAng);

      int mapX = (int)px, mapY = (int)py;
      double sideDistX, sideDistY;
      double deltaDistX = fabs(1.0 / (rx == 0 ? 1e-6 : rx));
      double deltaDistY = fabs(1.0 / (ry == 0 ? 1e-6 : ry));
      int stepX = (rx < 0) ? -1 : 1;
      int stepY = (ry < 0) ? -1 : 1;

      sideDistX = (rx < 0) ? (px - mapX) * deltaDistX : (mapX + 1.0 - px) * deltaDistX;
      sideDistY = (ry < 0) ? (py - mapY) * deltaDistY : (mapY + 1.0 - py) * deltaDistY;

      int hit = 0, side = 0;
      while (!hit) {
        if (sideDistX < sideDistY) { sideDistX += deltaDistX; mapX += stepX; side = 0; }
        else                        { sideDistY += deltaDistY; mapY += stepY; side = 1; }
        if (mapX < 0 || mapY < 0 || mapX >= MAPW || mapY >= MAPH) { hit = 1; break; }
        if (worldMap[mapY][mapX] > 0) hit = 1;
      }

      double perpDist = side ? (mapY - py + (1 - stepY)/2.0) / (ry==0?1e-6:ry)
                             : (mapX - px + (1 - stepX)/2.0) / (rx==0?1e-6:rx);
      if (perpDist < 1e-4) perpDist = 1e-4;
      depth[x] = perpDist;

      int lineH = (int)(H / perpDist);
      int start = -lineH/2 + H/2; if (start < 0) start = 0;
      int end   =  lineH/2 + H/2; if (end >= H) end = H-1;

      // distance + side shading
      double shade = 200.0 / (1.0 + 0.1 * perpDist);
      if (shade < 30.0) shade = 30.0; if (shade > 200.0) shade = 200.0;
      Uint8 c = (Uint8)shade; if (side) c = (Uint8)(c * 0.75);
      SDL_SetRenderDrawColor(ren, c, c, c, 255);
      SDL_RenderDrawLine(ren, x, start, x, end);
    }

    // --- simple minimap ---
    const int scale = 4; // pixels per tile on minimap
    int mx0 = 10, my0 = 10; // top-left of minimap
    for (int my = 0; my < MAPH; ++my) {
      for (int mx = 0; mx < MAPW; ++mx) {
        SDL_Color col = worldMap[my][mx] ? (SDL_Color){120,120,120,255} : (SDL_Color){25,25,25,255};
        draw_rect(ren, mx0 + mx*scale, my0 + my*scale, scale-1, scale-1, col);
      }
    }
    // player marker & facing
    draw_rect(ren, mx0 + (int)(px*scale)-2, my0 + (int)(py*scale)-2, 4, 4, (SDL_Color){255,50,50,255});
    int fx = (int)(mx0 + px*scale + cos(ang)*6);
    int fy = (int)(my0 + py*scale + sin(ang)*6);
    SDL_SetRenderDrawColor(ren, 255,50,50,255);
    SDL_RenderDrawLine(ren, mx0 + (int)(px*scale), my0 + (int)(py*scale), fx, fy);

    SDL_RenderPresent(ren);
  }

  SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit();
  return 0;
}
