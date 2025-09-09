#ifndef RAYS_H
#define RAYS_H

#include <stdbool.h>
#include <stdint.h>
#include "vector.h"

// Epsilon for intersection robustness
#ifndef RT_EPS
#define RT_EPS 1e-4f
#endif

typedef struct { Point3 origin; Vec3 dir; } Ray;

// Triangle primitive (single-sided by default)
typedef struct {
    Point3 v0, v1, v2;
    Vec3 e1, e2;
    Vec3 n_unit;
    Vec3 albedo;     // 0..1 per channel (for now, grayscale ok)
} Triangle;

typedef struct {
    bool   hit;
    float  t;          // ray parameter (distance along Ray.dir)
    Point3 p;          // hit position
    Vec3   n;          // geometric normal (unit)
    Vec3   albedo;     // 0..1 per channel (for now, grayscale ok)
} Hit;

typedef struct {
    Vec3  pos;        // light position
    Vec3  color;      // RGB in 0..1 (e.g., white = v3(1,1,1))
    float intensity; // luminous strength scalar (try 1.0..10.0 to start)
} Light;

// Möller–Trumbore ray-triangle; returns true on hit in (tmin, tmax]
bool ray_triangle_intersect(const Ray* r, const Triangle* tri,
                            float tmin, float tmax, Hit* out);

// Simple Lambert term → 0..255
uint8_t lambert_to_u8(float nDotL);

bool occluded_to_light(Point3 p, Vec3 n, Vec3 light_pos,
                       const Triangle* tris, int n_tris,
                       float bias);

// testing
static inline __attribute__((always_inline))
float mt_intersect_t(const Ray* __restrict r,
                     const Triangle* __restrict tri,
                     float tmin, float tmax)
{
    // Möller–Trumbore with predicate (avoid early returns)
    Vec3 p  = vcross(r->dir, tri->e2);
    float det = vdot(tri->e1, p);

    // det ~ 0 => parallel or tiny area
    bool ok = !(det > -RT_EPS && det < RT_EPS);

    float invDet = 1.0f / det;
    Vec3  tvec   = vsub(r->origin, tri->v0);
    float u      = vdot(tvec, p) * invDet;
    ok = ok && (u >= 0.0f && u <= 1.0f);

    Vec3  q      = vcross(tvec, tri->e1);
    float v      = vdot(r->dir, q) * invDet;
    ok = ok && (v >= 0.0f && (u + v) <= 1.0f);

    float t      = vdot(tri->e2, q) * invDet;
    ok = ok && (t > tmin && t <= tmax);

    return ok ? t : 1e30f; // sentinel for "no hit"
}

#endif
