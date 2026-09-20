COMMON_MAKEFILE := $(CURDIR)/$(lastword $(MAKEFILE_LIST))

DOCKER_COMPOSE_LINUX := docker-compose -f docker-compose.yml

docker-clean:
	$(DOCKER_COMPOSE) down --rmi local -v
	$(DOCKER_COMPOSE) rm -f -v

setup-linux:
	$(DOCKER_COMPOSE_LINUX) build

clean-linux: DOCKER_COMPOSE = $(DOCKER_COMPOSE_LINUX)
clean-linux: docker-clean

build-x64: clean-linux setup-linux
	$(DOCKER_COMPOSE_LINUX) up
