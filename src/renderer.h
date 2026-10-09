#ifndef RENDERER_H
#define RENDERER_H

#include "camera.h"
#include "entity_system.h"
#include "model.h"
#include "spline_entity.h"

void renderer_init(void);
void renderer_render_model_entity(
	const Entity *entity,
	const Camera *camera
);
void renderer_render_spline_entity(
	const Entity *entity,
	const Camera *camera
);
void renderer_draw_entities(
	const EntitySystem *system,
	const Camera *camera
);
void renderer_draw_text(int x, int y, const char *text);
void renderer_present(void);

#endif
