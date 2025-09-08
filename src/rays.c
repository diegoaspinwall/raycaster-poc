#include "rays.h"

bool ray_triangle_intersect(const Ray* r, const Triangle* tri,
                            double tmin, double tmax, Hit* out)
{
    // https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
    Vec3 p  = vcross(r->dir, tri->e2);
    double det = vdot(tri->e1, p);

    if (fabs(det) < RT_EPS) return false; // parallel or tiny area
    double invDet = 1.0 / det;

    Vec3 tvec = vsub(r->origin, tri->v0);
    double u = vdot(tvec, p) * invDet;
    if (u < 0.0 || u > 1.0) return false;

    Vec3 q = vcross(tvec, tri->e1);
    double v = vdot(r->dir, q) * invDet;
    if (v < 0.0 || u + v > 1.0) return false;

    double t = vdot(tri->e2, q) * invDet;
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

bool ray_triangle_intersect_any(const Ray* r, const Triangle* tri,
                            double tmin, double tmax)
{
    // just for shadow intersections
    Vec3 p  = vcross(r->dir, tri->e2);
    double det = vdot(tri->e1, p);

    if (fabs(det) < RT_EPS) return false; // parallel or tiny area
    double invDet = 1.0 / det;

    Vec3 tvec = vsub(r->origin, tri->v0);
    double u = vdot(tvec, p) * invDet;
    if (u < 0.0 || u > 1.0) return false;

    Vec3 q = vcross(tvec, tri->e1);
    double v = vdot(r->dir, q) * invDet;
    if (v < 0.0 || u + v > 1.0) return false;

    double t = vdot(tri->e2, q) * invDet;
    if (t <= tmin || t > tmax) return false;

    return true;
}

Ray camera_primary_ray(const Camera* cam, int x, int y, int w, int h)
{
    // NDC in [-1,1] with pixel center sampling
    double ndc_x = ( (x + 0.5) / (double)w ) * 2.0 - 1.0;
    double ndc_y = 1.0 - ( (y + 0.5) / (double)h ) * 2.0; // flip Y

    // Project to camera plane using vertical FOV
    double th = tan((cam->vfov_deg * M_PI / 180.0) * 0.5);
    double px = ndc_x * cam->aspect * th;
    double py = ndc_y * th;

    Vec3 dir = vnorm(vadd(cam->forward, vadd(vscale(cam->right, px),
                                             vscale(cam->up,    py))));

    return (Ray){ .origin = cam->pos, .dir = dir };
}

uint8_t lambert_to_u8(double x)
{
	if (x < 0.0) x = 0.0; if (x > 1.0) x = 1.0;
	return (uint8_t)(x * 255.0 + 0.5);
}

bool occluded_to_light(Point3 p, Vec3 n, Vec3 light_pos,
                       const Triangle* tris, int n_tris,
                       double bias)
{
    // Start the shadow ray a tiny distance above the surface to avoid self-hit (acne)
    Point3 origin = vadd(p, vscale(n, bias));

    Vec3  toL = vsub(light_pos, origin);
    double distL = vlen(toL);
    Vec3  dir = vscale(toL, 1.0 / (distL > 0.0 ? distL : 1.0)); // normalize safely

    Ray sray = (Ray){ origin, dir };

    // Any hit closer than the light blocks it
    double tmin = 1e-4;
    double tmax = distL - 1e-4;

    for (int i = 0; i < n_tris; ++i) {
        if (ray_triangle_intersect_any(&sray, &tris[i], tmin, tmax)) {
            return true; // something is between P and the light
        }
    }
    return false; // clear line-of-sight to light
}

