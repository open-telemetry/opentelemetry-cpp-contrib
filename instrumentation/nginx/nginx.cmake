include(ExternalProject)

if (NOT NGINX_VERSION)
  find_program(NGINX_BIN nginx REQUIRED)
  execute_process(COMMAND ${NGINX_BIN} -v
    ERROR_VARIABLE NGINX_VERSION_STRING
  )
 
  string(REGEX MATCH "[0-9]+\\.\[0-9]+\\.[0-9]+" NGINX_VER ${NGINX_VERSION_STRING})
else()
  set(NGINX_VER "1.18.0")
endif()

set(NGINX_VERSION ${NGINX_VER} CACHE STRING "Nginx version to compile against")
message(STATUS "Using nginx ${NGINX_VERSION}")

ExternalProject_Add(project_nginx
  URL "http://nginx.org/download/nginx-${NGINX_VERSION}.tar.gz"
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
