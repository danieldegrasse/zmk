cmake_minimum_required(VERSION 3.13.1)

if (CONFIG_ZMK_SETTINGS)

target_sources(app PRIVATE settings.c)

endif()
