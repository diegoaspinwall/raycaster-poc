#ifndef VECTOR_OPERATIONS_H
#define VECTOR_OPERATIONS_H

// #include <math.h>

// #define PI 3.1415926

typedef struct {
	double	x;
	double	y;
	double	z;
} vector;

typedef struct {
	double	x;
	double	y;
	double	z;
} point;

typedef struct {
	double	m11;
	double	m12;
	double	m13;
	double	m21;
	double	m22;
	double	m23;
	double	m31;
	double	m32;
	double	m33;
} matrix; // left to right, THEN top to bottom

double dot_product(vector *v1, vector *v2);
vector *cross_product(vector *v1, vector *v2);
vector *matrix_vector_multiply(matrix *m, vector *v)
vector *add_vectors(vector *v1, vector *v2);
vector *subtract_vectors(vector *v1, vector *v2);
vector *normalize(vector *v);
vector *two_points_to_vector(point *p1, point *p2);

#endif
