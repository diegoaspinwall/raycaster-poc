// Minimal multi-triangle ray tracer with a point light + hard shadows.
// Build: make

#include <SDL2/SDL.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "vector.h"
#include "rays.h"

#define W  1280
#define H  720
/*
#define W  640
#define H  360
*/
#define k  0.001f
#define VFOV_DEG 60

Triangle tris[] = {
    { .v0 = {  20.0,  20.0, 0.0 }, .v1 = {  20.0, -20.0, 2.0 }, .v2 = {  20.0,  20.0, 2.0 }, .albedo = { 1.0, 1.0, 1.0 } }, // +x wall
    { .v0 = {  20.0, -20.0, 2.0 }, .v1 = {  20.0,  20.0, 0.0 }, .v2 = {  20.0, -20.0, 0.0 }, .albedo = { 1.0, 1.0, 1.0 } },
    { .v0 = { -20.0, -20.0, 2.0 }, .v1 = { -20.0,  20.0, 0.0 }, .v2 = { -20.0,  20.0, 2.0 }, .albedo = { 1.0, 1.0, 1.0 } }, // -x wall
    { .v0 = { -20.0,  20.0, 0.0 }, .v1 = { -20.0, -20.0, 2.0 }, .v2 = { -20.0, -20.0, 0.0 }, .albedo = { 1.0, 1.0, 1.0 } },
    { .v0 = { -20.0,  20.0, 2.0 }, .v1 = {  20.0,  20.0, 0.0 }, .v2 = {  20.0,  20.0, 2.0 }, .albedo = { 1.0, 1.0, 1.0 } }, // +y wall
    { .v0 = {  20.0,  20.0, 0.0 }, .v1 = { -20.0,  20.0, 2.0 }, .v2 = { -20.0,  20.0, 0.0 }, .albedo = { 1.0, 1.0, 1.0 } },
    { .v0 = {  20.0, -20.0, 0.0 }, .v1 = { -20.0, -20.0, 2.0 }, .v2 = {  20.0, -20.0, 2.0 }, .albedo = { 1.0, 1.0, 1.0 } }, // -y wall
    { .v0 = { -20.0, -20.0, 2.0 }, .v1 = {  20.0, -20.0, 0.0 }, .v2 = { -20.0, -20.0, 0.0 }, .albedo = { 1.0, 1.0, 1.0 } },

    { .v0 = {  20.0,  20.0, 0.0 }, .v1 = { -20.0,  20.0, 0.0 }, .v2 = { -20.0, -20.0, 0.0 }, .albedo = { 1.0, 1.0, 1.0 } }, // floor
    { .v0 = {  20.0, -20.0, 0.0 }, .v1 = {  20.0,  20.0, 0.0 }, .v2 = { -20.0, -20.0, 0.0 }, .albedo = { 1.0, 1.0, 1.0 } },

    { .v0 = {  -3.0,  12.0, 0.0 }, .v1 = {  -3.0,  10.0, 9.0 }, .v2 = {  -3.0,  12.0, 9.0 }, .albedo = { 1.0, 1.0, 1.0 } }, // -x wall
    { .v0 = {  -3.0,  10.0, 9.0 }, .v1 = {  -3.0,  12.0, 0.0 }, .v2 = {  -3.0,  10.0, 0.0 }, .albedo = { 1.0, 1.0, 1.0 } },
    { .v0 = {   3.0,  10.0, 9.0 }, .v1 = {   3.0,  12.0, 0.0 }, .v2 = {   3.0,  12.0, 9.0 }, .albedo = { 1.0, 1.0, 1.0 } }, // +x wall
    { .v0 = {   3.0,  12.0, 0.0 }, .v1 = {   3.0,  10.0, 9.0 }, .v2 = {   3.0,  10.0, 0.0 }, .albedo = { 1.0, 1.0, 1.0 } },
    { .v0 = {  -3.0,  10.0, 9.0 }, .v1 = {   3.0,  10.0, 0.0 }, .v2 = {   3.0,  10.0, 9.0 }, .albedo = { 1.0, 1.0, 1.0 } }, // -y wall
    { .v0 = {   3.0,  10.0, 0.0 }, .v1 = {  -3.0,  10.0, 9.0 }, .v2 = {  -3.0,  10.0, 0.0 }, .albedo = { 1.0, 1.0, 1.0 } },
    { .v0 = {   3.0,  12.0, 0.0 }, .v1 = {  -3.0,  12.0, 9.0 }, .v2 = {   3.0,  12.0, 9.0 }, .albedo = { 1.0, 1.0, 1.0 } }, // +y wall
    { .v0 = {  -3.0,  12.0, 9.0 }, .v1 = {   3.0,  12.0, 0.0 }, .v2 = {  -3.0,  12.0, 0.0 }, .albedo = { 1.0, 1.0, 1.0 } },
};
const int n_tris = (int)(sizeof(tris)/sizeof(tris[0]));

static void build_tri_cache(void) {
    for (int i=0;i<n_tris;++i) {
        tris[i].e1 = vsub(tris[i].v1, tris[i].v0);
        tris[i].e2 = vsub(tris[i].v2, tris[i].v0);
        tris[i].n_unit = vnorm(vcross(tris[i].e1, tris[i].e2));
    }
}

// --------- NEW: per-frame camera packet ----------
typedef struct {
    Vec3 pos;         // camera origin
    Vec3 forward;     // unit vector
    Vec3 right;       // unit vector
    Vec3 up;          // unit vector
    float th;        // tan(vfov/2)
    float aspect;    // W/H
} CamParams;

typedef struct {
    uint32_t* fb;   // W*H ARGB32 framebuffer
    int y0, y1;     // [y0, y1) rows to render
    int pitch_px;   // = W
    CamParams cam;  // --------- precomputed camera ---------
    Light light;
} JobArgs;

static void render_rows(uint32_t* fb, int y0, int y1,
                        int pitch_px, const CamParams* cam, const Light* light)
{
    // constants for x stepping
    const float step = 2.f / (float)W;
    const float aspect_th = cam->aspect * cam->th;

    for (int y = y0; y < y1; ++y) {
        // NDC y and projected py are constant across the row
        float ndc_y = 1.f - ((y + 0.5f) * (2.f / (float)H));
        float py = ndc_y * cam->th;

        // start x at leftmost pixel center
        float ndc_x = -1.f + 0.5f * step;
        float px_val = ndc_x * aspect_th;

        uint32_t* dst = fb + y * pitch_px;

        for (int x = 0; x < W; ++x) {
            // Build primary ray direction from precomputed basis
            Vec3 dir = vadd(cam->forward,
                        vadd(vscale(cam->right, px_val),
                             vscale(cam->up,    py)));
            dir = vnorm(dir); // keep normalization for safety/consistency

            Ray ray = { .origin = cam->pos, .dir = dir };

            Hit best = {0};
            best.t = 1e30f;

            // Find closest intersection among all tris
            for (int i = 0; i < n_tris; ++i) {
                Hit h = {0};
                if (ray_triangle_intersect(&ray, &tris[i], 1e-4f, best.t, &h)) {
                    best = h;
                }
            }

            uint8_t r,g,b;
            if (best.hit) {
                // Shadow test
                bool blocked = occluded_to_light(best.p, best.n, light->pos, tris, n_tris, 1e-4f);
                if (blocked) {
                    r = g = b = 0; // in shadow: ambient only
                } else {
                    Vec3 L = vsub(light->pos, best.p);
                    float dist2 = vlen2(L);
                    L = vscale(L, 1.f / sqrtf(dist2));

                    float ndotl = vdot(best.n, L);
                    float diffuse = fmax(0.f, ndotl);

                    float atten = 1.f / (1.f + k*dist2);
                    float E = light->intensity * diffuse * atten; // light.intensity inlined for speed

                    float rr = best.albedo.x * light->color.x * E;
                    float gg = best.albedo.y * light->color.y * E;
                    float bb = best.albedo.z * light->color.z * E;

                    rr = rr < 0.f ? 0.f : (rr > 1.f ? 1.f : rr);
                    gg = gg < 0.f ? 0.f : (gg > 1.f ? 1.f : gg);
                    bb = bb < 0.f ? 0.f : (bb > 1.f ? 1.f : bb);

                    r = (uint8_t)(rr * 255.f + 0.5f);
                    g = (uint8_t)(gg * 255.f + 0.5f);
                    b = (uint8_t)(bb * 255.f + 0.5f);
                }
            } else {
                r = g = b = 50; // background
            }

            dst[x] = (0xFFu<<24) | (r<<16) | (g<<8) | b; // ARGB32

            // advance x terms
            ndc_x += step;
            px_val += step * aspect_th;
        }
    }
}

static void* worker(void* arg)
{
    JobArgs* a = (JobArgs*)arg;
    render_rows(a->fb, a->y0, a->y1, a->pitch_px, &a->cam, &a->light);
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

    float px = 0.f, py = 0.f;     // player position
    float azimuth = 0.f, elevation = 0.f;              // radians
    const float move = 5.f;       // units / s
    const float turn_lr = 2.f;       // rad / s
    const float turn_ud = 1.f;       // rad / s

    float light_ang = 0.f;                 // <-- NEW: light angle (radians)
    const float light_speed  = 0.05f;       // radians per second
    const float light_radius = 60.f;       // how wide the orbit is
    const float light_height = 40.f;       // z height of the light
    const Vec3  light_center = {0.f, 0.f, 0.f}; // orbit center in XY

    build_tri_cache();

    // --------- NEW: constants that never change ----------
    const float aspect = (float)W / (float)H;
    const float th = tanf((VFOV_DEG * 3.14159265358979323846f / 180.f) * 0.5f);

    uint64_t last = SDL_GetPerformanceCounter();
    bool running = true; SDL_Event e;
    while (running) {
        // --- timestep ---
        uint64_t now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last) / SDL_GetPerformanceFrequency();
        if (dt > 0.033f) dt = 0.033f; // clamp
        last = now;

        // --- input ---
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = false;
        }

        const Uint8* ks = SDL_GetKeyboardState(NULL);
        if (ks[SDL_SCANCODE_LEFT])  azimuth += turn_lr * dt;
        if (ks[SDL_SCANCODE_RIGHT]) azimuth -= turn_lr * dt;
        if (ks[SDL_SCANCODE_UP]) elevation += turn_ud * dt;
        if (ks[SDL_SCANCODE_DOWN]) elevation -= turn_ud * dt;
        const float half_pi = 1.57079632679f;
        const float max_pitch = half_pi - 1e-3f;
        if (elevation >  max_pitch) elevation =  max_pitch;
        if (elevation < -max_pitch) elevation = -max_pitch;

        float dx = cosf(azimuth), dy = sinf(azimuth);
        float mx = 0.f, my = 0.f;
        if (ks[SDL_SCANCODE_W]) { mx += dx * move * dt; my += dy * move * dt; }
        if (ks[SDL_SCANCODE_S]) { mx -= dx * move * dt; my -= dy * move * dt; }
        if (ks[SDL_SCANCODE_A]) { mx += -dy * move * dt; my +=  dx * move * dt; }
        if (ks[SDL_SCANCODE_D]) { mx +=  dy * move * dt; my += -dx * move * dt; }

        // --- collision ---
        // TODO
        px += mx;
        py += my;

        // --------- NEW: per-frame camera precompute ----------
        CamParams cam = {0};
        cam.pos = v3(px, py, 1.f);

	// Forward from yaw (azimuth) and pitch (elevation)
	Vec3 forward = vnorm(v3(
	    cosf(elevation) * cosf(azimuth),
	    cosf(elevation) * sinf(azimuth),
	    sinf(elevation)
	));

	Vec3 worldUp = v3(0.f, 0.f, 1.f);

	Vec3 right = vnorm(vcross(forward, worldUp));
	Vec3 up    = vnorm(vcross(right, forward));

        cam.forward = forward;
        cam.right   = right;
        cam.up      = up;
        cam.th      = th;      // constant
        cam.aspect  = aspect;  // constant

        // --- light ---
	// advance light angle
	light_ang += light_speed * dt;
	if (light_ang > 6.28318530718f) light_ang -= 6.28318530718f; // keep it bounded

	// compute light position on a circle around Z
	Vec3 light_pos = v3(
	    light_center.x + light_radius * cosf(light_ang),
	    light_center.y + light_radius * sinf(light_ang),
	    light_height
	);

	// build the light for this frame
	Light light = {
	    .pos = light_pos,
	    .color = v3(1.f, 1.f, 1.f),
	    .intensity = 5.f
	};

        // --- render ---
        // Split rows into T chunks
        pthread_t ths[64];
        JobArgs   ja[64];
        int rows_per = (H + T - 1) / T;
        for (int i = 0; i < T; ++i) {
            int y0 = i * rows_per;
            int y1 = y0 + rows_per;
            if (y1 > H) y1 = H;

            ja[i] = (JobArgs){
                .fb = fb, .y0 = y0, .y1 = y1, .pitch_px = W,
                .cam = cam,
                .light = light
            };
            pthread_create(&ths[i], NULL, worker, &ja[i]);
        }
        for (int i = 0; i < T; ++i) pthread_join(ths[i], NULL);

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
