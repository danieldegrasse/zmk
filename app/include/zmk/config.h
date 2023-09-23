/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

/*
 * ZMK keymap record. This is used by the USB layer to communicate
 * with the host PC
 */
struct zmk_keymap_record {
    uint32_t behavior_id;
    uint32_t param1;
    uint32_t param2;
};

/**
 * @brief Initializes the config subsystem
 *
 * Initializes the config subsystem for use with ZMK. This will also
 * enable the settings subsystem.
 * @retval 0 on success
 * @retval negative on error
 */
int zmk_config_init(void);

/**
 * @brief Get key record at index in layer
 *
 * Gets a key record in a given layer, at the given offset into the keymap
 * array.
 * @param layer_index: zmk layer index
 * @param key_index: index of key in layer
 * @param record: filled with key data
 * @return 0 on success, or negative on error
 */
int zmk_config_get_key_record(uint8_t layer_index, uint8_t key_index,
                                struct zmk_keymap_record *record);

/**
 * @brief Set key record at index in layer
 *
 * Sets a key record in a given layer, at the given offset into the keymap
 * array.
 * @param layer_index: zmk layer index
 * @param key_index: index of key in layer
 * @param record: contains key data to set
 * @return 0 on success, or negative on error
 */
int zmk_config_set_key_record(uint8_t layer_index, uint8_t key_index,
                                struct zmk_keymap_record *record);
