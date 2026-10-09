#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <psxgte.h>

#include "plane_entity.h"
#include "renderer.h"
#include "units.h"

#define PLANE_GRID_SIZE 12
#define PLANE_VERTICES_PER_SIDE (PLANE_GRID_SIZE + 1)
#define PLANE_VERTEX_COUNT \
	(PLANE_VERTICES_PER_SIDE * PLANE_VERTICES_PER_SIDE)
#define PLANE_TRIANGLE_COUNT (PLANE_GRID_SIZE * PLANE_GRID_SIZE * 2)

static SVECTOR plane_vertices[PLANE_VERTEX_COUNT];
static ModelTriangle plane_triangles[PLANE_TRIANGLE_COUNT];

static const SVECTOR plane_normals[] = {
	{ 0, -ONE, 0, 0 }
};

static const ModelMaterial plane_materials[] = {
	{ 112, 176, 112 },
	{  64, 112,  72 }
};

static const Model plane_model = {
	.vertices = plane_vertices,
	.normals = plane_normals,
	.materials = plane_materials,
	.triangles = plane_triangles,
	.vertex_count = PLANE_VERTEX_COUNT,
	.normal_count = 1,
	.material_count = 2,
	.triangle_count = PLANE_TRIANGLE_COUNT
};

static int model_initialized;

static void initialize_plane_model(void) {
	if (model_initialized) {
		return;
	}

	for (int row = 0; row < PLANE_VERTICES_PER_SIDE; row++) {
		for (int column = 0;
			column < PLANE_VERTICES_PER_SIDE;
			column++) {
			int index = row * PLANE_VERTICES_PER_SIDE + column;

			plane_vertices[index] = (SVECTOR) {
				WORLD_METERS(column - PLANE_GRID_SIZE / 2),
				0,
				WORLD_METERS(row - PLANE_GRID_SIZE / 2),
				0
			};
		}
	}

	for (int row = 0; row < PLANE_GRID_SIZE; row++) {
		for (int column = 0; column < PLANE_GRID_SIZE; column++) {
			int cell = row * PLANE_GRID_SIZE + column;
			int triangle = cell * 2;
			uint16_t top_left =
				row * PLANE_VERTICES_PER_SIDE + column;
			uint16_t top_right = top_left + 1;
			uint16_t bottom_left =
				top_left + PLANE_VERTICES_PER_SIDE;
			uint16_t bottom_right = bottom_left + 1;
			uint16_t material = (row + column) & 1;

			plane_triangles[triangle] = (ModelTriangle) {
				{ top_left, top_right, bottom_left },
				0,
				material
			};
			plane_triangles[triangle + 1] = (ModelTriangle) {
				{ top_right, bottom_right, bottom_left },
				0,
				material
			};
		}
	}

	model_initialized = 1;
}

void plane_entity_init(PlaneEntity *plane) {
	assert(plane != NULL);
	initialize_plane_model();
	model_entity_init(&plane->model_entity, &plane_model);
	entity_set_render_method(
		&plane->model_entity.entity,
		renderer_render_model_entity
	);
}

Entity *plane_entity_as_entity(PlaneEntity *plane) {
	assert(plane != NULL);
	return &plane->model_entity.entity;
}
