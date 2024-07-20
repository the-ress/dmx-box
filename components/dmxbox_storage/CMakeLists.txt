idf_component_register(
  SRCS
    dmxbox_storage.c
    effect_step_storage.c
    effect_storage.c
    private.c
  INCLUDE_DIRS include
  REQUIRES
    dmxbox_const
    nvs_flash
)
