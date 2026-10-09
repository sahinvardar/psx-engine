#ifndef SPLINE_ENTITY_H
#define SPLINE_ENTITY_H

#include <stdint.h>

#include "entity.h"
#include "spline.h"

typedef struct {
	Entity entity;
	Spline spline;
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t subdivisions_per_segment;
} SplineEntity;

typedef struct {
	Entity entity;
	Entity *target;
	const SplineEntity *path;
	TimeDelta duration;
	TimeDelta elapsed;
	int looping;
	int playing;
} SplineFollowerEntity;

void spline_entity_init(
	SplineEntity *entity,
	const SVECTOR *control_points,
	uint16_t control_point_count,
	int closed
);
void spline_entity_set_color(
	SplineEntity *entity,
	uint8_t r,
	uint8_t g,
	uint8_t b
);
void spline_entity_set_subdivisions(
	SplineEntity *entity,
	uint8_t subdivisions_per_segment
);
void spline_follower_entity_init(
	SplineFollowerEntity *follower,
	const SplineEntity *path,
	Entity *target,
	TimeDelta duration,
	int looping
);
void spline_follower_entity_play(SplineFollowerEntity *follower);
void spline_follower_entity_pause(SplineFollowerEntity *follower);
void spline_follower_entity_restart(SplineFollowerEntity *follower);

#endif
