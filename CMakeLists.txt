cmake_minimum_required(VERSION 3.21)

project(
	psx_hello_cube
	LANGUAGES C
	VERSION 1.0.0
	DESCRIPTION "A Psy-Q-style rotating cube for the original PlayStation"
)

psn00bsdk_add_executable(
	hello_cube
	GPREL
	src/camera.c
	src/cube_entity.c
	src/entity.c
	src/entity_system.c
	src/gamepad_entity.c
	src/game_time.c
	src/main.c
	src/model.c
	src/renderer.c
	src/spline.c
	src/spline_entity.c
)

target_compile_options(hello_cube PRIVATE
	-Wall
	-Wextra
	-Wpedantic
)

psn00bsdk_add_cd_image(
	disc
	hello_cube
	iso.xml
	DEPENDS hello_cube system.cnf
)
