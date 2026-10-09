#include <assert.h>
#include <stddef.h>

#include "model.h"

void model_entity_init(ModelEntity *entity, const Model *model) {
	assert(entity != NULL);
	assert(model != NULL);
	entity_init(&entity->entity);
	entity->model = model;
}
