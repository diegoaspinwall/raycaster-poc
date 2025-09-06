// Minimal multi-triangle ray tracer with a point light + hard shadows.
// Build: make

#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "vector.h"
#include "rays.h"

#define W  1280
#define H  720

Triangle tris[] = {
    { .v0 = {  20.0,  20.0, 0.0 }, .v1 = {  20.0, -20.0, 2.0 }, .v2 = {  20.0,  20.0, 2.0 }, .albedo = { 1.0, 1.0, 1.0 } }, // +x
    { .v0 = {  20.0, -20.0, 2.0 }, .v1 = {  20.0,  20.0, 0.0 }, .v2 = {  20.0, -20.0, 0.0 }, .albedo = { 1.0, 1.0, 1.0 } },
};
const int n_tris = (int)(sizeof(tris)/sizeof(tris[0]));

static void render_frame(uint32_t* pixels, int pitch)
{
    // Camera
    Camera cam = {0};
    cam.pos = v3(0.0, 0.0, 1.0);
    cam.forward = vnorm(v3(1.0, 0.0, 0.0));
    cam.up      = vnorm(v3(0.0, 0.0, 1.0));
    cam.right   = vnorm(vcross(cam.up, cam.forward));
    cam.vfov_deg = 60.0;
    cam.aspect   = (double)W / (double)H;

    // Point light in front of the quad
    Light light = {
        .pos = v3(0.7, 0.8, -0.8),
        .color = v3(1.0, 1.0, 1.0),   // white light
        .intensity = 1.0              // try 1..8 to see the effect
    };

    for (int y = 0; y < H; ++y) {
        uint32_t* row = (uint32_t*)((uint8_t*)pixels + y * pitch);
        for (int x = 0; x < W; ++x) {
            Ray ray = camera_primary_ray(&cam, x, y, W, H);

            Hit best = {0};
            best.t = 1e30;
            // Find closest intersection among all tris
            for (int i = 0; i < n_tris; ++i) {
                Hit h = {0};
                if (ray_triangle_intersect(&ray, &tris[i], 1e-4, best.t, &h)) {
                    best = h;
                }
            }

            uint8_t r,g,b;
            if (best.hit) {
                // Shadow test
                bool blocked = occluded_to_light(best.p, best.n, light.pos, tris, n_tris, 1e-4);
                if (blocked) {
                    r = g = b = 10; // in shadow: ambient only
                } else {
                    Vec3 L = vsub(light.pos, best.p);
                    double dist2 = vlen2(L);
                    L = vscale(L, 1.0 / sqrt(dist2));

                    double ndotl = vdot(best.n, L);
                    double diffuse = fmax(0.0, ndotl);

                    // Irradiance from light: intensity scales brightness
                    double atten = 1.0 / (1.0 + 0.002*dist2);

                    double E = light.intensity * diffuse * atten;

                    double rr = best.albedo.x * light.color.x * E;
                    double gg = best.albedo.y * light.color.y * E;
                    double bb = best.albedo.z * light.color.z * E;

                    rr = rr < 0.0 ? 0.0 : (rr > 1.0 ? 1.0 : rr);
                    gg = gg < 0.0 ? 0.0 : (gg > 1.0 ? 1.0 : gg);
                    bb = bb < 0.0 ? 0.0 : (bb > 1.0 ? 1.0 : bb);

                    r = (uint8_t)(rr * 255.0 + 0.5);
                    g = (uint8_t)(gg * 255.0 + 0.5);
                    b = (uint8_t)(bb * 255.0 + 0.5);
                }
            } else {
                r = g = b = 25; // background
            }

            row[x] = (0xFFu<<24) | (r<<16) | (g<<8) | b; // ARGB32
        }
    }
}

int main(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Raytrace — shadows", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, 0);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, W, H);

    bool running = true; SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = false;
        }

        void* pixels; int pitch;
        SDL_LockTexture(tex, NULL, &pixels, &pitch);
        render_frame((uint32_t*)pixels, pitch);
        SDL_UnlockTexture(tex);

        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
