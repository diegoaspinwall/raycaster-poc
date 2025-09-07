// Minimal multi-triangle ray tracer with a point light + hard shadows.
// Build: make

#include <SDL2/SDL.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "vector.h"
#include "rays.h"

#define W  1280
#define H  720
/*
#define W  640
#define H  360
*/
#define k  0.001
#define VFOV_DEG 60

Triangle tris[] = {
    { .v0 = {  20.0,  20.0, 0.0 }, .v1 = {  20.0, -20.0, 2.0 }, .v2 = {  20.0,  20.0, 2.0 }, .albedo = { 1.0, 1.0, 1.0 } }, // +x
    { .v0 = {  20.0, -20.0, 2.0 }, .v1 = {  20.0,  20.0, 0.0 }, .v2 = {  20.0, -20.0, 0.0 }, .albedo = { 1.0, 1.0, 1.0 } },
    { .v0 = { -20.0, -20.0, 2.0 }, .v1 = { -20.0,  20.0, 0.0 }, .v2 = { -20.0,  20.0, 2.0 }, .albedo = { 1.0, 1.0, 1.0 } }, // -x
    { .v0 = { -20.0,  20.0, 0.0 }, .v1 = { -20.0, -20.0, 2.0 }, .v2 = { -20.0, -20.0, 0.0 }, .albedo = { 1.0, 1.0, 1.0 } },
    { .v0 = { -20.0,  20.0, 2.0 }, .v1 = {  20.0,  20.0, 0.0 }, .v2 = {  20.0,  20.0, 2.0 }, .albedo = { 1.0, 1.0, 1.0 } }, // +y
    { .v0 = {  20.0,  20.0, 0.0 }, .v1 = { -20.0,  20.0, 2.0 }, .v2 = { -20.0,  20.0, 0.0 }, .albedo = { 1.0, 1.0, 1.0 } },
    { .v0 = {  20.0, -20.0, 0.0 }, .v1 = { -20.0, -20.0, 2.0 }, .v2 = {  20.0, -20.0, 2.0 }, .albedo = { 1.0, 1.0, 1.0 } }, // -y
    { .v0 = { -20.0, -20.0, 2.0 }, .v1 = {  20.0, -20.0, 0.0 }, .v2 = { -20.0, -20.0, 0.0 }, .albedo = { 1.0, 1.0, 1.0 } },
    { .v0 = {  20.0,  20.0, 0.0 }, .v1 = { -20.0,  20.0, 0.0 }, .v2 = { -20.0, -20.0, 0.0 }, .albedo = { 1.0, 1.0, 1.0 } }, // floor
    { .v0 = {  20.0, -20.0, 0.0 }, .v1 = {  20.0,  20.0, 0.0 }, .v2 = { -20.0, -20.0, 0.0 }, .albedo = { 1.0, 1.0, 1.0 } },
};
const int n_tris = (int)(sizeof(tris)/sizeof(tris[0]));

static void build_tri_cache(void) {
    for (int i=0;i<n_tris;++i) {
        tris[i].e1 = vsub(tris[i].v1, tris[i].v0);
        tris[i].e2 = vsub(tris[i].v2, tris[i].v0);
        tris[i].n_unit = vnorm(vcross(tris[i].e1, tris[i].e2));
    }
}

typedef struct {
    uint32_t* fb;   // W*H ARGB32 framebuffer
    int y0, y1;     // [y0, y1) rows to render
    int pitch_px;   // = W
    // capture what render_frame needs:
    double pos_x, pos_y, azimuth;
} JobArgs;

static void render_rows(uint32_t* fb, int y0, int y1,
                        int pitch_px, double pos_x, double pos_y, double azimuth)
{
    // Camera
    Camera cam = {0};
    cam.pos = v3(pos_x, pos_y, 1.0);
    cam.forward = vnorm(v3(cos(azimuth), sin(azimuth), 0.0));

    Vec3 worldUp = v3(0.0, 0.0, 1.0);
    cam.right   = vnorm(vcross(cam.forward, worldUp));
    cam.up      = vnorm(vcross(cam.right, cam.forward));

    cam.vfov_deg = VFOV_DEG;
    cam.aspect   = (double)W / (double)H;

    // Point light in front of the quad
    Light light = {
        .pos = v3(-200.0, 200.0, 40.0),
        .color = v3(1.0, 1.0, 1.0),   // white light
        .intensity = 300.0              // try 1..8 to see the effect
    };

    for (int y = y0; y < y1; ++y) {
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
                    r = g = b = 20; // in shadow: ambient only
                } else {
                    Vec3 L = vsub(light.pos, best.p);
                    double dist2 = vlen2(L);
                    L = vscale(L, 1.0 / sqrt(dist2));

                    double ndotl = vdot(best.n, L);
                    double diffuse = fmax(0.0, ndotl);

                    // Irradiance from light: intensity scales brightness
                    double atten = 1.0 / (1.0 + k*dist2);

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
                r = g = b = 50; // background
            }

            fb[y*pitch_px + x] = (0xFFu<<24) | (r<<16) | (g<<8) | b; // ARGB32
        }
    }
}

static void* worker(void* arg)
{
    JobArgs* a = (JobArgs*)arg;
    render_rows(a->fb, a->y0, a->y1, a->pitch_px, a->pos_x, a->pos_y, a->azimuth);
    return NULL;
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

    uint32_t* fb = malloc(W * H * sizeof(uint32_t));
    if (!fb) { fprintf(stderr, "fb alloc failed\n"); return 1;}

    // choose thread count
    int T = SDL_GetCPUCount();
    if (T < 1) T = 1;
    if (T > 12) T = 12;

    double px = 0.0, py = 0.0;     // player position
    double ang = 0.0;              // radians
    const double move = 5.0;       // units / s
    const double turn = 2.0;       // rad / s

    build_tri_cache();

    uint64_t last = SDL_GetPerformanceCounter();
    bool running = true; SDL_Event e;
    while (running) {
        // --- timestep ---
        uint64_t now = SDL_GetPerformanceCounter();
        double dt = (double)(now - last) / SDL_GetPerformanceFrequency();
        if (dt > 0.033) dt = 0.033; // clamp
        last = now;

        // --- input ---
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = false;
        }

        const Uint8* ks = SDL_GetKeyboardState(NULL);
        if (ks[SDL_SCANCODE_LEFT])  ang += turn * dt;
        if (ks[SDL_SCANCODE_RIGHT]) ang -= turn * dt;

        double dx = cos(ang), dy = sin(ang);
        double mx = 0, my = 0;
        if (ks[SDL_SCANCODE_W]) { mx += dx * move * dt; my += dy * move * dt; }
        if (ks[SDL_SCANCODE_S]) { mx -= dx * move * dt; my -= dy * move * dt; }
        if (ks[SDL_SCANCODE_A]) { mx += -dy * move * dt; my +=  dx * move * dt; }
        if (ks[SDL_SCANCODE_D]) { mx +=  dy * move * dt; my += -dx * move * dt; }

        // --- collision ---
        // TODO
        px += mx;
        py += my;

        // --- render ---
        // Split rows into T chunks
        pthread_t th[64];
        JobArgs   ja[64];
        int rows_per = (H + T - 1) / T;
        for (int i = 0; i < T; ++i) {
            int y0 = i * rows_per;
            int y1 = y0 + rows_per;
            if (y1 > H) y1 = H;

            ja[i] = (JobArgs){
                .fb = fb, .y0 = y0, .y1 = y1, .pitch_px = W,
                .pos_x = px, .pos_y = py, .azimuth = ang
            };
            pthread_create(&th[i], NULL, worker, &ja[i]);
        }
        for (int i = 0; i < T; ++i) pthread_join(th[i], NULL);

        // Upload once on the main thread
        SDL_UpdateTexture(tex, NULL, fb, W * 4);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
    }

    free(fb);

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
