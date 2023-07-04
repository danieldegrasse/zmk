/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */


#pragma once

/**
 * @brief initializes settings subsystem.
 *
 * Initializes the settings subsystem. This function should be called from
 * the main() init point, and will set up any resources needed by the settings
 * subsystem
 */
void zmk_settings_init(void);



/*
 * The below functions are implemented by each settings transport
 * to allow the settings subsystem to send binary data to the host.
 */

/*
 * Used by the settings subsystem to initialize a
 * settings transport method, to communicate with host PC.
 */
void settings_transport_init(void);

/*
 * Writes a buffer payload to the host PC
 * @param buf: buffer of data to write
 * @param len: length of buffer in bytes
 * @retval number of bytes written, or negative value on error.
 */
int settings_transport_write(const uint8_t *buf, size_t len);

/*
 * Reads a buffer payload from the host PC
 * @param buf: buffer to read into
 * @param len: length of buffer in bytes
 * @retval number of bytes read, or negative value on error.
 */
int settings_transport_read(uint8_t *buf, size_t len);
