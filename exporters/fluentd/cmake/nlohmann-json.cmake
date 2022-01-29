if("${nlohmann-json}" STREQUAL "")
    set(nlohmann-json "v3.9.1")
endif()
include(ExternalProject)
ExternalProject_Add(nlohmann_json_download
    PREFIX nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/nlohmann_json"
    GIT_TAG
        "${nlohmann-json}"
    GIT_SHALLOW 1
    UPDATE_COMMAND ""
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
      -DJSON_BuildTests=OFF
      -DJSON_Install=ON
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    TEST_AFTER_INSTALL
        0
    DOWNLOAD_NO_PROGRESS
        1
    LOG_CONFIGURE
        1
    LOG_BUILD
        1
    LOG_INSTALL
        1
)

ExternalProject_Get_Property(nlohmann_json_download INSTALL_DIR)
SET(NLOHMANN_JSON_INCLUDE_DIR ${INSTALL_DIR}/nlohmann_json/src/nlohmann_json_download/single_include)
add_library(nlohmann_json_ INTERFACE)
target_include_directories(nlohmann_json_ INTERFACE
    "$<BUILD_INTERFACE:${NLOHMANN_JSON_INCLUDE_DIR}>"
    "$<INSTALL_INTERFACE:include>")
add_dependencies(nlohmann_json_ nlohmann_json_download)
add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json_)

install(
  TARGETS nlohmann_json_
  EXPORT "${PROJECT_NAME}-target"
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
