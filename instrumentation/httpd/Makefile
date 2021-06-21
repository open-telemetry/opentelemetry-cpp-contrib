

BUILD_IMAGE_BZL=opentelemetry-httpd-mod-dev
BUILD_IMAGE_CMAKE=opentelemetry-httpd-mod-dev-cmake
CONTAINER_NAME=otel-httpd
CONTAINER_NAME_CMAKE=otel-httpd-cmake

.PHONY: help build clean devsh rmcnt rming start stop

all: help

help:
	@echo Possible targets are:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

build-bazel: ## build docker image with development tools
	docker build -t $(BUILD_IMAGE_BZL) .

build-cmake: ## build docker image with development tools (cmake version)
	docker build -t $(BUILD_IMAGE_CMAKE) -f Dockerfile.cmake  .

start-bazel: build-bazel ## start new docker container
	docker run -v "$(PWD)/src":/mnt/host --network host --privileged --name "$(CONTAINER_NAME)" -it $(BUILD_IMAGE_BZL) /bin/bash -l

start-cmake: build-cmake ## start new docker container (cmake version)
	docker run -v "$(PWD)/src":/mnt/host --network host --privileged --name "$(CONTAINER_NAME_CMAKE)" -it $(BUILD_IMAGE_CMAKE) /bin/bash -l

start: start-bazel

rmcnt: ## remove docker container
	docker container rm $(CONTAINER_NAME) || true

rmcnt-cmake: ## remove docker container (cmake version)
	docker container rm $(CONTAINER_NAME_CMAKE) || true

rmimg: ## remove docker image
	docker image rm $(BUILD_IMAGE_BZL)

clean: rmcnt rmimg ## clean both docker container and image

devsh: ## attach to existing container with shell (developer shell)
	if [ "docker container inspect (CONTAINER_NAME)" ]; then \
		docker exec -it $(CONTAINER_NAME) bash; \
	else \
		$(MAKE) start;                                           \
	fi;

devsh-cmake: ## attach to existing container with shell (developer shell, cmake)
	if [ "docker container inspect (CONTAINER_NAME_CMAKE)" ]; then \
		docker exec -it $(CONTAINER_NAME_CMAKE) bash; \
	else \
		$(MAKE) start;                                           \
	fi;
