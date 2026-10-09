#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <psxgte.h>

#include "spline.h"

static int32_t catmull_rom_coordinate(
	int32_t p0,
	int32_t p1,
	int32_t p2,
	int32_t p3,
	int32_t parameter
) {
	int64_t parameter_squared =
		(int64_t) parameter * parameter / ONE;
	int64_t parameter_cubed =
		parameter_squared * parameter / ONE;
	int64_t result =
		2 * (int64_t) p1
		+ ((-p0 + p2) * (int64_t) parameter / ONE)
		+ (
			(2 * p0 - 5 * p1 + 4 * p2 - p3)
			* parameter_squared
			/ ONE
		)
		+ (
			(-p0 + 3 * p1 - 3 * p2 + p3)
			* parameter_cubed
			/ ONE
		);

	result /= 2;
	assert(result >= INT32_MIN && result <= INT32_MAX);
	return (int32_t) result;
}

static uint16_t control_point_index(
	const Spline *spline,
	int32_t index
) {
	int32_t count = spline->control_point_count;

	if (spline->closed) {
		index %= count;
		if (index < 0) {
			index += count;
		}
		return (uint16_t) index;
	}
	if (index < 0) {
		return 0;
	}
	if (index >= count) {
		return (uint16_t) (count - 1);
	}
	return (uint16_t) index;
}

void spline_init(
	Spline *spline,
	const SVECTOR *control_points,
	uint16_t control_point_count,
	int closed
) {
	assert(spline != NULL);
	assert(control_points != NULL);
	assert(control_point_count >= 2);
	spline->control_points = control_points;
	spline->control_point_count = control_point_count;
	spline->closed = closed != 0;
}

uint16_t spline_segment_count(const Spline *spline) {
	assert(spline != NULL);
	return spline->closed
		? spline->control_point_count
		: (uint16_t) (spline->control_point_count - 1);
}

void spline_sample(
	const Spline *spline,
	SplineParameter parameter,
	VECTOR *position
) {
	uint16_t segment_count;
	uint16_t segment;
	uint32_t segment_fraction;
	int32_t local_parameter;
	const SVECTOR *p0;
	const SVECTOR *p1;
	const SVECTOR *p2;
	const SVECTOR *p3;

	assert(spline != NULL);
	assert(position != NULL);
	assert(parameter <= SPLINE_PARAMETER_ONE);

	segment_count = spline_segment_count(spline);
	if (spline->closed && parameter == SPLINE_PARAMETER_ONE) {
		parameter = 0;
	}

	if (parameter == SPLINE_PARAMETER_ONE) {
		segment = segment_count - 1;
		local_parameter = ONE;
	} else {
		uint64_t scaled = (uint64_t) parameter * segment_count;

		segment = (uint16_t) (scaled / SPLINE_PARAMETER_ONE);
		segment_fraction = (uint32_t) (
			scaled % SPLINE_PARAMETER_ONE
		);
		local_parameter = (int32_t) (
			(uint64_t) segment_fraction * ONE
			/ SPLINE_PARAMETER_ONE
		);
	}

	p0 = &spline->control_points[
		control_point_index(spline, (int32_t) segment - 1)
	];
	p1 = &spline->control_points[
		control_point_index(spline, segment)
	];
	p2 = &spline->control_points[
		control_point_index(spline, (int32_t) segment + 1)
	];
	p3 = &spline->control_points[
		control_point_index(spline, (int32_t) segment + 2)
	];

	position->vx = catmull_rom_coordinate(
		p0->vx,
		p1->vx,
		p2->vx,
		p3->vx,
		local_parameter
	);
	position->vy = catmull_rom_coordinate(
		p0->vy,
		p1->vy,
		p2->vy,
		p3->vy,
		local_parameter
	);
	position->vz = catmull_rom_coordinate(
		p0->vz,
		p1->vz,
		p2->vz,
		p3->vz,
		local_parameter
	);
}
