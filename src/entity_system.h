#ifndef ENTITY_SYSTEM_H
#define ENTITY_SYSTEM_H

#include <stdint.h>

#include "entity.h"

#define ENTITY_SYSTEM_CAPACITY 64

typedef enum {
	ENTITY_SYSTEM_SUCCESS,
	ENTITY_SYSTEM_ALREADY_REGISTERED,
	ENTITY_SYSTEM_NOT_REGISTERED,
	ENTITY_SYSTEM_FULL
} EntitySystemResult;

typedef struct {
	Entity *entities[ENTITY_SYSTEM_CAPACITY];
	uint16_t count;
} EntitySystem;

void entity_system_init(EntitySystem *system);
EntitySystemResult entity_system_register(
	EntitySystem *system,
	Entity *entity
);
EntitySystemResult entity_system_deregister(
	EntitySystem *system,
	Entity *entity
);
Entity *entity_system_get(const EntitySystem *system, uint16_t slot);
void entity_system_run(EntitySystem *system, TimeDelta delta_time);

#endif
