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
	@expected="$$(shasum -a 256 Dockerfile | awk '{ print $$1 }')"; \
	actual="$$(docker image inspect \
		--format '{{ index .Config.Labels "dev.psx-engine.dockerfile-sha" }}' \
		$(IMAGE) 2>/dev/null || true)"; \
	if [ "$$actual" = "$$expected" ]; then \
		echo "Using existing toolchain image $(IMAGE)"; \
	else \
		echo "Building toolchain image $(IMAGE)"; \
		docker build \
			--platform $(PLATFORM) \
			--label "dev.psx-engine.dockerfile-sha=$$expected" \
			--tag $(IMAGE) \
			.; \
	fi

configure: toolchain
	$(DOCKER_RUN) cmake --preset default .

build: toolchain
	$(DOCKER_RUN) sh -c 'cmake --preset default . && cmake --build --preset default'

exe: toolchain
	$(DOCKER_RUN) sh -c 'cmake --preset default . && cmake --build --preset default --target psx-engine'

intellisense:
	./scripts/setup-intellisense.sh

run: exe
	@test -x "$(DUCKSTATION)" || (echo "DuckStation not found at $(DUCKSTATION)" >&2; exit 1)
	"$(DUCKSTATION)" -batch -fastboot -nofullscreen -- "$(CURDIR)/build/psx-engine.exe"

watch:
	DUCKSTATION="$(DUCKSTATION)" ./scripts/watch.sh

clean: toolchain
	$(DOCKER_RUN) cmake -E remove_directory build
