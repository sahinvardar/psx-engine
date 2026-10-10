cmake_minimum_required(VERSION 3.21)

project(
	psx_engine
	LANGUAGES C
	VERSION 1.0.0
	DESCRIPTION "A small entity-based engine for the original PlayStation"
)

set(
	PSX_LICENSE_FILE
	""
	CACHE FILEPATH
	"Region-specific license data dumped from a legally owned PlayStation disc"
)
option(
	PSX_CUSTOM_BOOT_CUBE
	"Replace the dumped disc's boot logo with a generated cube TMD"
	ON
)

if(NOT PSX_LICENSE_FILE AND EXISTS "${PROJECT_SOURCE_DIR}/license.dat")
	set(PSX_LICENSE_FILE "${PROJECT_SOURCE_DIR}/license.dat")
endif()

set(PSX_LICENSE_DEPENDENCY "")
if(PSX_LICENSE_FILE)
	get_filename_component(
		PSX_LICENSE_FILE
		"${PSX_LICENSE_FILE}"
		ABSOLUTE
	)
	if(NOT EXISTS "${PSX_LICENSE_FILE}")
		message(FATAL_ERROR "PSX license file not found: ${PSX_LICENSE_FILE}")
	endif()

	if(PSX_CUSTOM_BOOT_CUBE)
		find_package(Python3 REQUIRED COMPONENTS Interpreter)
		set(
			PSX_GENERATED_LICENSE
			"${PROJECT_BINARY_DIR}/license-cube.dat"
		)
		add_custom_command(
			OUTPUT "${PSX_GENERATED_LICENSE}"
			COMMAND
				"${Python3_EXECUTABLE}"
				"${PROJECT_SOURCE_DIR}/scripts/generate-cube-license.py"
				"${PSX_LICENSE_FILE}"
				"${PSX_GENERATED_LICENSE}"
			DEPENDS
				"${PSX_LICENSE_FILE}"
				"${PROJECT_SOURCE_DIR}/scripts/generate-cube-license.py"
			COMMENT "Generating custom cube boot logo"
			VERBATIM
		)
		set(PSX_LICENSE_FILE "${PSX_GENERATED_LICENSE}")
		set(PSX_LICENSE_DEPENDENCY "${PSX_GENERATED_LICENSE}")
	endif()

	file(TO_CMAKE_PATH "${PSX_LICENSE_FILE}" PSX_LICENSE_FILE)
	set(PSX_LICENSE_ELEMENT "<license file=\"${PSX_LICENSE_FILE}\" />")
else()
	set(PSX_LICENSE_ELEMENT "<!-- No region license data supplied. -->")
endif()

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
	DEPENDS
		psx-engine
		system.cnf
		${PSX_LICENSE_DEPENDENCY}
)
