#ifndef PHYSICS_H
#define PHYSICS_H

#include <stdint.h>
#include <psxgte.h>

#include "entity.h"

#define PHYSICS_SYSTEM_CAPACITY 32

typedef enum {
	PHYSICS_BODY_STATIC,
	PHYSICS_BODY_DYNAMIC
} PhysicsBodyType;

typedef enum {
	PHYSICS_SYSTEM_SUCCESS,
	PHYSICS_SYSTEM_ALREADY_REGISTERED,
	PHYSICS_SYSTEM_NOT_REGISTERED,
	PHYSICS_SYSTEM_FULL
} PhysicsSystemResult;

typedef struct {
	VECTOR normal;
	int32_t penetration;
} PhysicsContact;

struct PhysicsBody;

typedef void (*PhysicsCollisionCallback)(
	struct PhysicsBody *body,
	struct PhysicsBody *other,
	const PhysicsContact *contact
);

typedef struct PhysicsBody {
	Entity *entity;
	VECTOR half_extents;
	VECTOR velocity;
	VECTOR accumulated_force;
	VECTOR velocity_remainder;
	VECTOR position_remainder;
	int32_t mass;
	TimeDelta horizontal_damping;
	PhysicsBodyType type;
	PhysicsCollisionCallback on_collision;
	void *user_data;
	int enabled;
} PhysicsBody;

typedef struct {
	PhysicsBody *bodies[PHYSICS_SYSTEM_CAPACITY];
	VECTOR gravity;
	uint16_t count;
} PhysicsSystem;

void physics_system_init(PhysicsSystem *system);
void physics_system_set_gravity(
	PhysicsSystem *system,
	int32_t x,
	int32_t y,
	int32_t z
);
PhysicsSystemResult physics_system_register(
	PhysicsSystem *system,
	PhysicsBody *body
);
PhysicsSystemResult physics_system_deregister(
	PhysicsSystem *system,
	PhysicsBody *body
);
void physics_system_step(PhysicsSystem *system, TimeDelta delta_time);

void physics_body_init(
	PhysicsBody *body,
	Entity *entity,
	PhysicsBodyType type,
	int32_t half_width,
	int32_t half_height,
	int32_t half_depth
);
void physics_body_set_velocity(
	PhysicsBody *body,
	int32_t x,
	int32_t y,
	int32_t z
);
void physics_body_set_mass(PhysicsBody *body, int32_t mass);
void physics_body_set_horizontal_damping(
	PhysicsBody *body,
	TimeDelta damping
);
void physics_body_add_force(
	PhysicsBody *body,
	int32_t x,
	int32_t y,
	int32_t z
);
void physics_body_add_local_force(
	PhysicsBody *body,
	int32_t x,
	int32_t y,
	int32_t z
);
void physics_body_set_collision_callback(
	PhysicsBody *body,
	PhysicsCollisionCallback callback,
	void *user_data
);

#endif
