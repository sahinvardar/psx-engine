IMAGE := psx-hello-toolchain:0.24
PLATFORM := linux/amd64
DUCKSTATION ?= /Applications/DuckStation.app/Contents/MacOS/DuckStation
DOCKER_RUN := docker run --rm --platform $(PLATFORM) \
	--user "$(shell id -u):$(shell id -g)" \
	--env HOME=/tmp \
	--volume "$(CURDIR):/workspace" \
	--workdir /workspace \
	$(IMAGE)

.PHONY: all build clean configure exe intellisense run toolchain watch

all: build

toolchain:
	docker build --platform $(PLATFORM) --tag $(IMAGE) .

configure: toolchain
	$(DOCKER_RUN) cmake --preset default .

build: toolchain
	$(DOCKER_RUN) sh -c 'cmake --preset default . && cmake --build --preset default'

exe: toolchain
	$(DOCKER_RUN) sh -c 'cmake --preset default . && cmake --build --preset default --target hello_cube'

intellisense:
	./scripts/setup-intellisense.sh

run: exe
	@test -x "$(DUCKSTATION)" || (echo "DuckStation not found at $(DUCKSTATION)" >&2; exit 1)
	"$(DUCKSTATION)" -batch -fastboot -nofullscreen -- "$(CURDIR)/build/hello_cube.exe"

watch:
	DUCKSTATION="$(DUCKSTATION)" ./scripts/watch.sh

clean: toolchain
	$(DOCKER_RUN) cmake -E remove_directory build
