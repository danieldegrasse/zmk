/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/events/position_state_changed.h>

#ifdef CONFIG_ZMK_SETTINGS_KEYMAP_LAYERS
#define ZMK_KEYMAP_LAYERS_LEN CONFIG_ZMK_SETTINGS_KEYMAP_LAYERS
#else
#define ZMK_LAYER_CHILD_LEN_PLUS_ONE(node) 1 +
#define ZMK_KEYMAP_LAYERS_LEN                                                                      \
    (DT_FOREACH_CHILD(DT_INST(0, zmk_keymap), ZMK_LAYER_CHILD_LEN_PLUS_ONE) 0)
#endif

typedef uint32_t zmk_keymap_layers_state_t;

uint8_t zmk_keymap_layer_default();
zmk_keymap_layers_state_t zmk_keymap_layer_state();
bool zmk_keymap_layer_active(uint8_t layer);
uint8_t zmk_keymap_highest_layer_active();
int zmk_keymap_layer_activate(uint8_t layer);
int zmk_keymap_layer_deactivate(uint8_t layer);
int zmk_keymap_layer_toggle(uint8_t layer);
int zmk_keymap_layer_to(uint8_t layer);
const char *zmk_keymap_layer_label(uint8_t layer);

int zmk_keymap_position_state_changed(uint8_t source, uint32_t position, bool pressed,
                                      int64_t timestamp);

#define ZMK_KEYMAP_EXTRACT_BINDING(idx, drv_inst)                                                  \
    {                                                                                              \
        .behavior_dev = DT_PROP(DT_PHANDLE_BY_IDX(drv_inst, bindings, idx), label),                \
        .param1 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(drv_inst, bindings, idx, param1), (0),        \
                              (DT_PHA_BY_IDX(drv_inst, bindings, idx, param1))),                   \
        .param2 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(drv_inst, bindings, idx, param2), (0),        \
                              (DT_PHA_BY_IDX(drv_inst, bindings, idx, param2))),                   \
    }
