include(ExternalProject)

set(NGINX_VER "1.18.0" CACHE STRING "Nginx version to compile against")

ExternalProject_Add(project_nginx
  URL "http://nginx.org/download/nginx-${NGINX_VER}.tar.gz"
  PREFIX "nginx"
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND ./configure --with-compat
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)

set(NGINX_DIR "${CMAKE_BINARY_DIR}/nginx/src/project_nginx")

set(NGINX_INCLUDE_DIRS
  ${NGINX_DIR}/objs
  ${NGINX_DIR}/src/core
  ${NGINX_DIR}/src/os/unix
  ${NGINX_DIR}/src/event
  ${NGINX_DIR}/src/http
  ${NGINX_DIR}/src/http/modules
)
