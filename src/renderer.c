#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <inline_c.h>

#include "renderer.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define OT_LENGTH 256
#define PACKET_BUFFER_SIZE 16384

typedef struct
{
	DISPENV display;
	DRAWENV draw;
	uint32_t ordering_table[OT_LENGTH];
	uint32_t packets[PACKET_BUFFER_SIZE / sizeof(uint32_t)];
} RenderBuffer;

typedef struct
{
	RenderBuffer buffers[2];
	uint8_t *next_packet;
	int active_buffer;
} RenderContext;

static MATRIX color_matrix = {
		.m = {
				{ONE, 0, 0},
				{ONE, 0, 0},
				{ONE, 0, 0}},
		.t = {0, 0, 0}};

static MATRIX light_matrix = {
		.m = {
				{-2048, -2048, -2048},
				{0, 0, 0},
				{0, 0, 0}},
		.t = {0, 0, 0}};

static RenderContext context;

static void commit_primitive(void *primitive, int depth, size_t size)
{
	RenderBuffer *buffer = &context.buffers[context.active_buffer];

	addPrim(&buffer->ordering_table[depth], primitive);
	context.next_packet += size;
	assert(
			context.next_packet <= (uint8_t *)buffer->packets + sizeof(buffer->packets));
}

void renderer_init(void)
{
	ResetGraph(0);
	FntLoad(960, 0);
	InitGeom();
	gte_SetGeomOffset(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
	gte_SetGeomScreen(SCREEN_WIDTH / 2);
	gte_SetBackColor(48, 48, 48);
	gte_SetColorMatrix(&color_matrix);

	SetDefDrawEnv(
			&context.buffers[0].draw,
			0,
			0,
			SCREEN_WIDTH,
			SCREEN_HEIGHT);
	SetDefDispEnv(
			&context.buffers[0].display,
			0,
			0,
			SCREEN_WIDTH,
			SCREEN_HEIGHT);
	SetDefDrawEnv(
			&context.buffers[1].draw,
			0,
			SCREEN_HEIGHT,
			SCREEN_WIDTH,
			SCREEN_HEIGHT);
	SetDefDispEnv(
			&context.buffers[1].display,
			0,
			SCREEN_HEIGHT,
			SCREEN_WIDTH,
			SCREEN_HEIGHT);

	for (int index = 0; index < 2; index++)
	{
		setRGB0(&context.buffers[index].draw, 16, 20, 32);
		context.buffers[index].draw.isbg = 1;
		context.buffers[index].draw.dtd = 1;
	}

	context.active_buffer = 0;
	context.next_packet = (uint8_t *)context.buffers[0].packets;
	ClearOTagR(context.buffers[0].ordering_table, OT_LENGTH);
	SetDispMask(1);
}

void renderer_render_model_entity(
		const Entity *entity,
		const Camera *camera)
{
	const ModelEntity *model_entity = (const ModelEntity *)entity;
	const Model *model;
	MATRIX model_matrix;
	MATRIX view_matrix;
	MATRIX model_view_matrix;
	MATRIX transformed_light_matrix;

	assert(camera != NULL);
	assert(entity != NULL);
	model = model_entity->model;
	assert(model != NULL);
	assert(model->vertices != NULL);
	assert(model->normals != NULL);
	assert(model->materials != NULL);
	assert(model->triangles != NULL);

	entity_build_world_matrix(&model_entity->entity, &model_matrix);
	camera_build_view_matrix(camera, &view_matrix);
	CompMatrixLV(&view_matrix, &model_matrix, &model_view_matrix);
	MulMatrix0(
			&light_matrix,
			&model_matrix,
			&transformed_light_matrix);
	gte_SetRotMatrix(&model_view_matrix);
	gte_SetTransMatrix(&model_view_matrix);
	gte_SetLightMatrix(&transformed_light_matrix);

	for (uint16_t index = 0; index < model->triangle_count; index++)
	{
		const ModelTriangle *triangle_data = &model->triangles[index];
		const ModelMaterial *material;
		POLY_F3 *triangle = (POLY_F3 *)context.next_packet;
		int32_t visibility;
		int32_t depth;

		memset(triangle, 0, sizeof(POLY_F3));

		assert(triangle_data->vertex_indices[0] < model->vertex_count);
		assert(triangle_data->vertex_indices[1] < model->vertex_count);
		assert(triangle_data->vertex_indices[2] < model->vertex_count);
		assert(triangle_data->normal_index < model->normal_count);
		assert(triangle_data->material_index < model->material_count);

		gte_ldv3(
				&model->vertices[triangle_data->vertex_indices[0]],
				&model->vertices[triangle_data->vertex_indices[1]],
				&model->vertices[triangle_data->vertex_indices[2]]);
		gte_rtpt();
		gte_nclip();
		gte_stopz(&visibility);

		if (visibility < 0)
		{
			continue;
		}

		setPolyF3(triangle);
		gte_stsxy0(&triangle->x0);
		gte_stsxy1(&triangle->x1);
		gte_stsxy2(&triangle->x2);

		gte_avsz3();
		gte_stotz(&depth);
		depth >>= 2;

		if (depth <= 0 || depth >= OT_LENGTH)
		{
			continue;
		}

		material = &model->materials[triangle_data->material_index];
		setRGB0(
				triangle,
				material->r,
				material->g,
				material->b);
		gte_ldrgb(&triangle->r0);
		gte_ldv0(&model->normals[triangle_data->normal_index]);
		gte_nccs();
		gte_strgb(&triangle->r0);

		commit_primitive(triangle, depth, sizeof(POLY_F3));
	}
}

static SVECTOR spline_position_to_vertex(const VECTOR *position)
{
	SVECTOR vertex;

	assert(position->vx >= INT16_MIN && position->vx <= INT16_MAX);
	assert(position->vy >= INT16_MIN && position->vy <= INT16_MAX);
	assert(position->vz >= INT16_MIN && position->vz <= INT16_MAX);
	vertex.vx = (int16_t)position->vx;
	vertex.vy = (int16_t)position->vy;
	vertex.vz = (int16_t)position->vz;
	vertex.pad = 0;
	return vertex;
}

void renderer_render_spline_entity(
		const Entity *entity,
		const Camera *camera)
{
	const SplineEntity *spline_entity = (const SplineEntity *)entity;
	MATRIX path_matrix;
	MATRIX view_matrix;
	MATRIX model_view_matrix;
	uint32_t line_count;

	assert(entity != NULL);
	assert(camera != NULL);
	assert(spline_entity->subdivisions_per_segment > 0);

	entity_build_world_matrix(entity, &path_matrix);
	camera_build_view_matrix(camera, &view_matrix);
	CompMatrixLV(&view_matrix, &path_matrix, &model_view_matrix);
	gte_SetRotMatrix(&model_view_matrix);
	gte_SetTransMatrix(&model_view_matrix);

	line_count =
			(uint32_t)spline_segment_count(&spline_entity->spline) * spline_entity->subdivisions_per_segment;

	for (uint32_t index = 0; index < line_count; index++)
	{
		VECTOR start_position;
		VECTOR end_position;
		SVECTOR start_vertex;
		SVECTOR end_vertex;
		LINE_F2 *line = (LINE_F2 *)context.next_packet;
		int32_t start_depth;
		int32_t end_depth;
		int32_t depth;

		spline_sample(
				&spline_entity->spline,
				(SplineParameter)((uint64_t)index * SPLINE_PARAMETER_ONE / line_count),
				&start_position);
		spline_sample(
				&spline_entity->spline,
				(SplineParameter)((uint64_t)(index + 1) * SPLINE_PARAMETER_ONE / line_count),
				&end_position);
		start_vertex = spline_position_to_vertex(&start_position);
		end_vertex = spline_position_to_vertex(&end_position);

		setLineF2(line);
		setRGB0(
				line,
				spline_entity->r,
				spline_entity->g,
				spline_entity->b);

		gte_ldv0(&start_vertex);
		gte_rtps();
		gte_stsxy(&line->x0);
		gte_stsz(&start_depth);

		gte_ldv0(&end_vertex);
		gte_rtps();
		gte_stsxy(&line->x1);
		gte_stsz(&end_depth);

		if (start_depth <= 0 || end_depth <= 0)
		{
			continue;
		}
		depth = ((start_depth + end_depth) / 2) >> 2;
		if (depth <= 0 || depth >= OT_LENGTH)
		{
			continue;
		}

		commit_primitive(line, depth, sizeof(LINE_F2));
	}
}

void renderer_draw_entities(
		const EntitySystem *system,
		const Camera *camera)
{
	assert(system != NULL);
	assert(camera != NULL);

	for (uint16_t slot = 0; slot < ENTITY_SYSTEM_CAPACITY; slot++)
	{
		const Entity *entity = entity_system_get(system, slot);

		if (entity != NULL && entity->render != NULL)
		{
			entity->render(entity, camera);
		}
	}
}

void renderer_draw_text(int x, int y, const char *text)
{
	RenderBuffer *buffer = &context.buffers[context.active_buffer];

	assert(text != NULL);
	context.next_packet = FntSort(
			&buffer->ordering_table[0],
			context.next_packet,
			x,
			y,
			text);
	assert(
			context.next_packet <= (uint8_t *)buffer->packets + sizeof(buffer->packets));
}

void renderer_present(void)
{
	RenderBuffer *draw_buffer;
	RenderBuffer *display_buffer;

	DrawSync(0);
	VSync(0);

	draw_buffer = &context.buffers[context.active_buffer];
	display_buffer = &context.buffers[context.active_buffer ^ 1];

	PutDispEnv(&display_buffer->display);
	DrawOTagEnv(
			&draw_buffer->ordering_table[OT_LENGTH - 1],
			&draw_buffer->draw);

	context.active_buffer ^= 1;
	context.next_packet = (uint8_t *)display_buffer->packets;
	ClearOTagR(display_buffer->ordering_table, OT_LENGTH);
}
