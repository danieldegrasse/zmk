cmake_minimum_required(VERSION 3.13.1)

if (CONFIG_ZMK_SETTINGS)

# Setup python command to run during build, to generate CBOR encode/decode code
set(py_command
  zcbor
  code
  -c ${CMAKE_CURRENT_LIST_DIR}/settings.cddl
  --output-cmake ${PROJECT_BINARY_DIR}/cbor_settings.cmake
  -t keymap command
  -e
  -d
  )

execute_process(
  COMMAND ${py_command}
  COMMAND_ERROR_IS_FATAL ANY
)

include(${PROJECT_BINARY_DIR}/cbor_settings.cmake)
target_link_libraries(cbor_settings PRIVATE zephyr_interface)
target_link_libraries(app PRIVATE cbor_settings)

if (CONFIG_ZMK_SETTINGS_BACKEND_CDC_ACM)
target_sources(app PRIVATE settings_cdc_acm.c)
endif()

target_sources(app PRIVATE settings.c)

endif()
