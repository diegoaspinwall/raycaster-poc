#ifndef RAYS_H
#define RAYS_H

#include <stdbool.h>
#include "vector_operations.h"

typdef struct {
	bool	happened;
	double	distance;
	double	angle; // or normal? or dot product?
	point*	location;
} collision;

typdef struct {
	point*	start;
	vector*	direction;
} ray;

collision *ray_triangle_intersect(ray *r, triangle *t);
unsigned char dot_product_to_color(double dot_product);
vector *pixel_position_to_ray(unsigned short x, unsigned short y);
/* Also needs some information about direction of camera. Maybe should
go in a different file, relating to camera position and movement. */

#endif
