#ifndef RAYS_H
#define RAYS_H

#include <stdbool.h>
#include <stdint.h>
#include "vector.h"

// Epsilon for intersection robustness
#ifndef RT_EPS
#define RT_EPS 1e-8
#endif

typedef struct { Point3 origin; Vec3 dir; } Ray;

// Triangle primitive (single-sided by default)
typedef struct {
    Point3 v0, v1, v2;
    Vec3   albedo;     // 0..1 per channel (for now, grayscale ok)
} Triangle;

typedef struct {
    bool   hit;
    double t;          // ray parameter (distance along Ray.dir)
    Point3 p;          // hit position
    Vec3   n;          // geometric normal (unit)
    Vec3   albedo;     // 0..1 per channel (for now, grayscale ok)
} Hit;

// Simple pinhole camera
typedef struct {
    Point3 pos;
    Vec3   forward;    // normalized
    Vec3   right;      // normalized
    Vec3   up;         // normalized (right-handed basis)
    double vfov_deg;   // vertical field of view in degrees
    double aspect;     // width / height
} Camera;

typedef struct {
    Vec3  pos;        // light position
    Vec3  color;      // RGB in 0..1 (e.g., white = v3(1,1,1))
    double intensity; // luminous strength scalar (try 1.0..10.0 to start)
} Light;

// Möller–Trumbore ray-triangle; returns true on hit in (tmin, tmax]
bool ray_triangle_intersect(const Ray* r, const Triangle* tri,
                            double tmin, double tmax, Hit* out);

// Generate primary ray through pixel center (x,y) in [0..w-1],[0..h-1]
Ray camera_primary_ray(const Camera* cam, int x, int y, int w, int h);

// Simple Lambert term → 0..255
uint8_t lambert_to_u8(double nDotL);

bool occluded_to_light(Point3 p, Vec3 n, Vec3 light_pos,
                       const Triangle* tris, int n_tris,
                       double bias);

#endif
