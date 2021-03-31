

BUILD_IMAGE=opentelemetry-httpd-mod-dev
CONTAINER_NAME=otel-httpd

.PHONY: help build clean devsh rmcnt rming start stop

all: help

help:
	@echo Possible targets are:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

build: ## build docker image with development tools
	docker build -t $(BUILD_IMAGE) .

start: build ## start new docker container
	docker run -v "$(PWD)/src":/mnt/host --network host --privileged --name "$(CONTAINER_NAME)" -it $(BUILD_IMAGE) /bin/bash -l

startifnotrunning:


# stop: ## stop docker image

rmcnt: ## remove docker container
	docker container rm $(CONTAINER_NAME) || true

rmimg: ## remove docker image
	docker image rm $(BUILD_IMAGE)

clean: rmcnt rmimg ## clean both docker container and image

devsh: ## attach to existing container with shell (developer shell)
	if [ "docker container inspect (CONTAINER_NAME)" ]; then \
		docker exec -it $(CONTAINER_NAME) bash; \
	else \
		$(MAKE) start;                                           \
	fi;
