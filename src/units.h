#ifndef UNITS_H
#define UNITS_H

#include <stdint.h>

#define WORLD_UNITS_PER_METER 100

#define WORLD_METERS(value) \
	((int32_t) (value) * WORLD_UNITS_PER_METER)

#define WORLD_CENTIMETERS(value) \
	((int32_t) (value) * WORLD_UNITS_PER_METER / 100)

#define WORLD_MILLIMETERS(value) \
	((int32_t) (value) * WORLD_UNITS_PER_METER / 1000)

#define WORLD_TO_CENTIMETERS(value) \
	((int32_t) (value) * 100 / WORLD_UNITS_PER_METER)

#endif
