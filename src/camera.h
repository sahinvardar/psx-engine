#ifndef CAMERA_H
#define CAMERA_H

#include <psxgte.h>

#include "entity.h"

typedef struct Camera {
	Entity entity;
} Camera;

void camera_init(Camera *camera);
void camera_build_view_matrix(const Camera *camera, MATRIX *view_matrix);

#endif
