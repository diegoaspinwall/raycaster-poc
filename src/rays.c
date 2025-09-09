#include "rays.h"

bool ray_triangle_intersect(const Ray* r, const Triangle* tri,
                            float tmin, float tmax, Hit* out)
{
    // https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
    Vec3 p  = vcross(r->dir, tri->e2);
    float det = vdot(tri->e1, p);

    if (fabsf(det) < RT_EPS) return false; // parallel or tiny area
    float invDet = 1.f / det;

    Vec3 tvec = vsub(r->origin, tri->v0);
    float u = vdot(tvec, p) * invDet;
    if (u < 0.f || u > 1.f) return false;

    Vec3 q = vcross(tvec, tri->e1);
    float v = vdot(r->dir, q) * invDet;
    if (v < 0.f || u + v > 1.f) return false;

    float t = vdot(tri->e2, q) * invDet;
    if (t <= tmin || t > tmax) return false;

    if (out) {
        out->hit = true;
        out->t   = t;
        out->p   = vadd(r->origin, vscale(r->dir, t));
        // out->n   = vnorm(vcross(e1, e2));
        out->n   = tri->n_unit;
        out->albedo =  tri->albedo;
    }
    return true;
}

bool ray_triangle_intersect_any(const Ray* __restrict r, const Triangle* __restrict tri,
                            float tmin, float tmax)
{
    // just for shadow intersections
    Vec3 p  = vcross(r->dir, tri->e2);
    float det = vdot(tri->e1, p);

    if (fabsf(det) < RT_EPS) return false; // parallel or tiny area
    float invDet = 1.f / det;

    Vec3 tvec = vsub(r->origin, tri->v0);
    float u = vdot(tvec, p) * invDet;
    if (u < 0.f || u > 1.f) return false;

    Vec3 q = vcross(tvec, tri->e1);
    float v = vdot(r->dir, q) * invDet;
    if (v < 0.f || u + v > 1.f) return false;

    float t = vdot(tri->e2, q) * invDet;
    if (t <= tmin || t > tmax) return false;

    return true;
}

uint8_t lambert_to_u8(float x)
{
	if (x < 0.f) x = 0.f; if (x > 1.f) x = 1.f;
	return (uint8_t)(x * 255.f + 0.5f);
}

bool occluded_to_light(Point3 p, Vec3 n, Vec3 light_pos,
                       const Triangle* __restrict tris, int n_tris,
                       float bias)
{
    // Start the shadow ray a tiny distance above the surface to avoid self-hit (acne)
    Point3 origin = vadd(p, vscale(n, bias));

    Vec3  toL = vsub(light_pos, origin);
    float distL = vlen(toL);
    Vec3  dir = vscale(toL, 1.f / (distL > 0.f ? distL : 1.f)); // normalize safely

    Ray sray = (Ray){ origin, dir };

    // Any hit closer than the light blocks it
    float tmin = 1e-4f;
    float tmax = distL - 1e-4f;

    for (int i = 0; i < n_tris; ++i) {
        if (ray_triangle_intersect_any(&sray, &tris[i], tmin, tmax)) {
            return true; // something is between P and the light
        }
    }
    return false; // clear line-of-sight to light
}

