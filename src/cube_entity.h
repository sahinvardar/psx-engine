#ifndef CUBE_ENTITY_H
#define CUBE_ENTITY_H

#include <stdint.h>

#include "model.h"

typedef struct {
	ModelEntity model_entity;
	int32_t pitch_rate;
	int32_t yaw_rate;
	int32_t roll_rate;
} CubeEntity;

void cube_entity_init(CubeEntity *cube);
Entity *cube_entity_as_entity(CubeEntity *cube);
void cube_entity_set_spin(
	CubeEntity *cube,
	int32_t pitch_rate,
	int32_t yaw_rate,
	int32_t roll_rate
);
void cube_entity_stop_spin(CubeEntity *cube);

#endif
