#ifndef SPLINE_H
#define SPLINE_H

#include <stdint.h>
#include <psxgte.h>

#define SPLINE_PARAMETER_ONE 65536

typedef uint32_t SplineParameter;

typedef struct {
	const SVECTOR *control_points;
	uint16_t control_point_count;
	int closed;
} Spline;

void spline_init(
	Spline *spline,
	const SVECTOR *control_points,
	uint16_t control_point_count,
	int closed
);
uint16_t spline_segment_count(const Spline *spline);
void spline_sample(
	const Spline *spline,
	SplineParameter parameter,
	VECTOR *position
);

#endif
