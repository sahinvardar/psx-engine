#ifndef PLANE_ENTITY_H
#define PLANE_ENTITY_H

#include "model.h"

typedef struct {
	ModelEntity model_entity;
} PlaneEntity;

void plane_entity_init(PlaneEntity *plane);
Entity *plane_entity_as_entity(PlaneEntity *plane);

#endif
