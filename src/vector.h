#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct { double x, y, z; } Vec3;
typedef Vec3 Point3; // semantic alias

// Constructors/helpers
static inline Vec3  v3(double x, double y, double z) { return (Vec3){x,y,z}; }
static inline Vec3  vadd(Vec3 a, Vec3 b)   { return v3(a.x+b.x, a.y+b.y, a.z+b.z); }
static inline Vec3  vsub(Vec3 a, Vec3 b)   { return v3(a.x-b.x, a.y-b.y, a.z-b.z); }
static inline Vec3  vscale(Vec3 a, double s){ return v3(a.x*s, a.y*s, a.z*s); }
static inline double vdot(Vec3 a, Vec3 b)  { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline Vec3  vcross(Vec3 a, Vec3 b) { return v3(a.y*b.z - a.z*b.y,
                                                      a.z*b.x - a.x*b.z,
                                                      a.x*b.y - a.y*b.x); }
static inline double vlen2(Vec3 a)         { return vdot(a,a); }
static inline double vlen(Vec3 a)          { return sqrt(vlen2(a)); }
static inline Vec3  vnorm(Vec3 a)          { double L=vlen(a); return (L>0)? vscale(a,1.0/L) : a; }

// Optional 3x3 for transforms (row-major)
typedef struct { double m11,m12,m13, m21,m22,m23, m31,m32,m33; } Mat3;

static inline Vec3 m3_mul_v3(Mat3 m, Vec3 v) {
    return v3(m.m11*v.x + m.m12*v.y + m.m13*v.z,
              m.m21*v.x + m.m22*v.y + m.m23*v.z,
              m.m31*v.x + m.m32*v.y + m.m33*v.z);
}

#endif
