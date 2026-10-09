#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <psxgte.h>

#include "renderer.h"
#include "spline_entity.h"

static void place_target_on_path(SplineFollowerEntity *follower) {
	SplineParameter parameter;
	VECTOR local_position;
	VECTOR world_position;
	MATRIX path_matrix;

	parameter = (SplineParameter) (
		(int64_t) follower->elapsed * SPLINE_PARAMETER_ONE
		/ follower->duration
	);
	spline_sample(&follower->path->spline, parameter, &local_position);
	entity_build_world_matrix(&follower->path->entity, &path_matrix);
	ApplyMatrixLV(&path_matrix, &local_position, &world_position);
	entity_set_position(
		follower->target,
		world_position.vx,
		world_position.vy,
		world_position.vz
	);
}

static void spline_follower_run(
	Entity *entity,
	TimeDelta delta_time
) {
	SplineFollowerEntity *follower = (SplineFollowerEntity *) entity;

	assert(delta_time >= 0);
	if (!follower->playing) {
		return;
	}

	follower->elapsed += delta_time;
	if (follower->elapsed >= follower->duration) {
		if (follower->looping) {
			follower->elapsed %= follower->duration;
		} else {
			follower->elapsed = follower->duration;
			follower->playing = 0;
		}
	}
	place_target_on_path(follower);
}

void spline_entity_init(
	SplineEntity *entity,
	const SVECTOR *control_points,
	uint16_t control_point_count,
	int closed
) {
	assert(entity != NULL);
	entity_init(&entity->entity);
	spline_init(
		&entity->spline,
		control_points,
		control_point_count,
		closed
	);
	entity->r = 64;
	entity->g = 255;
	entity->b = 255;
	entity->subdivisions_per_segment = 8;
	entity_set_render_method(
		&entity->entity,
		renderer_render_spline_entity
	);
}

void spline_entity_set_color(
	SplineEntity *entity,
	uint8_t r,
	uint8_t g,
	uint8_t b
) {
	assert(entity != NULL);
	entity->r = r;
	entity->g = g;
	entity->b = b;
}

void spline_entity_set_subdivisions(
	SplineEntity *entity,
	uint8_t subdivisions_per_segment
) {
	assert(entity != NULL);
	assert(subdivisions_per_segment > 0);
	entity->subdivisions_per_segment = subdivisions_per_segment;
}

void spline_follower_entity_init(
	SplineFollowerEntity *follower,
	const SplineEntity *path,
	Entity *target,
	TimeDelta duration,
	int looping
) {
	assert(follower != NULL);
	assert(path != NULL);
	assert(target != NULL);
	assert(duration > 0);
	entity_init(&follower->entity);
	follower->target = target;
	follower->path = path;
	follower->duration = duration;
	follower->elapsed = 0;
	follower->looping = looping != 0;
	follower->playing = 1;
	entity_set_run_method(&follower->entity, spline_follower_run);
	place_target_on_path(follower);
}

void spline_follower_entity_play(SplineFollowerEntity *follower) {
	assert(follower != NULL);
	follower->playing = 1;
}

void spline_follower_entity_pause(SplineFollowerEntity *follower) {
	assert(follower != NULL);
	follower->playing = 0;
}

void spline_follower_entity_restart(SplineFollowerEntity *follower) {
	assert(follower != NULL);
	follower->elapsed = 0;
	follower->playing = 1;
	place_target_on_path(follower);
}
