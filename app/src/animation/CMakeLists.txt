cmake_minimum_required(VERSION 3.20.0)

# Include base animation drivers
target_sources(app PRIVATE animation.c)
target_sources(app PRIVATE color.c)

# Add animation drivers based on which are enabled
target_sources_ifdef(CONFIG_ZMK_ANIMATION_COMPOSE app PRIVATE animation_compose.c)
target_sources_ifdef(CONFIG_ZMK_ANIMATION_CONTROL app PRIVATE animation_control.c)
target_sources_ifdef(CONFIG_ZMK_ANIMATION_RIPPLE app PRIVATE animation_ripple.c)
target_sources_ifdef(CONFIG_ZMK_ANIMATION_SOLID app PRIVATE animation_solid.c)
target_sources_ifdef(CONFIG_ZMK_ANIMATION_INDICATOR app PRIVATE animation_indicator.c)

