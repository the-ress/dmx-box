idf_component_register(
  SRCS ""
  INCLUDE_DIRS ""
)

set(NPM_PROJECT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(NPM_BUILD_DIR   "${CMAKE_CURRENT_BINARY_DIR}")
set(NPM_DIST_DIR    "${NPM_BUILD_DIR}/dist")

ExternalProject_Add(
  dmxbox_ui_npm
  SOURCE_DIR "${NPM_PROJECT_DIR}"

  # UPDATE_COMMAND npm install
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND ""

  BUILD_ALWAYS FALSE
  BUILD_IN_SOURCE TRUE
  BUILD_COMMAND npm run clean
  COMMAND npm run build
        --
        --no-cache
        --no-source-maps
        --log-level info
        --dist-dir "${NPM_DIST_DIR}"

  INSTALL_COMMAND ""
)

spiffs_create_partition_image(
  www
  "${NPM_DIST_DIR}"
  DEPS dmxbox_ui_npm
  FLASH_IN_PROJECT
)
