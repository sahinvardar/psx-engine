#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "entity_system.h"

void entity_system_init(EntitySystem *system) {
	assert(system != NULL);

	for (uint16_t slot = 0; slot < ENTITY_SYSTEM_CAPACITY; slot++) {
		system->entities[slot] = NULL;
	}
	system->count = 0;
}

EntitySystemResult entity_system_register(
	EntitySystem *system,
	Entity *entity
) {
	int available_slot = -1;

	assert(system != NULL);
	assert(entity != NULL);

	for (uint16_t slot = 0; slot < ENTITY_SYSTEM_CAPACITY; slot++) {
		if (system->entities[slot] == entity) {
			return ENTITY_SYSTEM_ALREADY_REGISTERED;
		}
		if (system->entities[slot] == NULL && available_slot < 0) {
			available_slot = slot;
		}
	}

	if (available_slot < 0) {
		return ENTITY_SYSTEM_FULL;
	}

	system->entities[available_slot] = entity;
	system->count++;
	return ENTITY_SYSTEM_SUCCESS;
}

EntitySystemResult entity_system_deregister(
	EntitySystem *system,
	Entity *entity
) {
	assert(system != NULL);
	assert(entity != NULL);

	for (uint16_t slot = 0; slot < ENTITY_SYSTEM_CAPACITY; slot++) {
		if (system->entities[slot] == entity) {
			system->entities[slot] = NULL;
			system->count--;
			return ENTITY_SYSTEM_SUCCESS;
		}
	}

	return ENTITY_SYSTEM_NOT_REGISTERED;
}

Entity *entity_system_get(const EntitySystem *system, uint16_t slot) {
	assert(system != NULL);
	assert(slot < ENTITY_SYSTEM_CAPACITY);
	return system->entities[slot];
}

void entity_system_run(EntitySystem *system, TimeDelta delta_time) {
	assert(system != NULL);
	assert(delta_time >= 0);

	for (uint16_t slot = 0; slot < ENTITY_SYSTEM_CAPACITY; slot++) {
		Entity *entity = system->entities[slot];

		if (entity != NULL && entity->run != NULL) {
			entity->run(entity, delta_time);
		}
	}
}
