#ifndef MODEL_H
#define MODEL_H

#include <stdint.h>
#include <psxgte.h>

#include "entity.h"

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} ModelMaterial;

typedef struct {
	uint16_t vertex_indices[3];
	uint16_t normal_index;
	uint16_t material_index;
} ModelTriangle;

typedef struct {
	const SVECTOR *vertices;
	const SVECTOR *normals;
	const ModelMaterial *materials;
	const ModelTriangle *triangles;
	uint16_t vertex_count;
	uint16_t normal_count;
	uint16_t material_count;
	uint16_t triangle_count;
} Model;

typedef struct {
	Entity entity;
	const Model *model;
} ModelEntity;

void model_entity_init(ModelEntity *entity, const Model *model);

#endif
