cmake_minimum_required(VERSION 3.21)

project(
	psx_engine
	LANGUAGES C
	VERSION 1.0.0
	DESCRIPTION "A small entity-based engine for the original PlayStation"
)

psn00bsdk_add_executable(
	psx-engine
	GPREL
	src/camera.c
	src/cube_entity.c
	src/entity.c
	src/entity_system.c
	src/gamepad_entity.c
	src/game_time.c
	src/main.c
	src/model.c
	src/physics.c
	src/plane_entity.c
	src/renderer.c
	src/spline.c
	src/spline_entity.c
)

target_compile_options(psx-engine PRIVATE
	-Wall
	-Wextra
	-Wpedantic
)

psn00bsdk_add_cd_image(
	disc
	psx-engine
	iso.xml
	DEPENDS psx-engine system.cnf
)
