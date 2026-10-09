#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <psxgte.h>

#include "camera.h"

static int32_t transform_position_component(
	const int16_t row[3],
	const VECTOR *position
) {
	int64_t transformed =
		(int64_t) row[0] * position->vx
		+ (int64_t) row[1] * position->vy
		+ (int64_t) row[2] * position->vz;

	transformed = -(transformed / ONE);
	assert(transformed >= INT32_MIN && transformed <= INT32_MAX);
	return (int32_t) transformed;
}

void camera_init(Camera *camera) {
	assert(camera != NULL);
	entity_init(&camera->entity);
}

void camera_build_view_matrix(const Camera *camera, MATRIX *view_matrix) {
	MATRIX camera_world_matrix;

	assert(camera != NULL);
	assert(view_matrix != NULL);
	entity_build_world_matrix(&camera->entity, &camera_world_matrix);

	for (int row = 0; row < 3; row++) {
		for (int column = 0; column < 3; column++) {
			view_matrix->m[row][column] =
				camera_world_matrix.m[column][row];
		}
		view_matrix->t[row] = transform_position_component(
			view_matrix->m[row],
			&camera->entity.transform.position
		);
	}
}
