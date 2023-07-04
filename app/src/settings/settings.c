/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/settings.h>
#include <cbor_settings_encode.h>
#include <cbor_settings_decode.h>

uint8_t settings_read_buf[CONFIG_ZMK_SETTINGS_MAX_READ];
uint32_t settings_read_offset;
uint8_t settings_write_buf[CONFIG_ZMK_SETTINGS_MAX_WRITE];

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);
struct k_work zmk_settings_work;

#define SETTINGS_CMD_SIZE (4U) /* Settings command header is 4 bytes */

/*
 * This function responds to the read keymap command with
 * a keymap CBOR structure representing the currently programmed
 * keymap data.
 */
static void settings_read_keymap(void) {
	int res;
	size_t write_len, cmd_write_len;
	struct command cmd = {
		._command_type_choice = _command_type_read_keymap_res,
	};
	/* Send a dummy keymap */
	uint8_t keys[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xAA, 0xBB};
	struct keymap map = {
		._keymap_id = 0,
		._keymap_map = {
			.value = keys,
			.len = sizeof(keys),
		},
	};

	res = cbor_encode_keymap(settings_write_buf, sizeof(settings_write_buf),
			&map, &write_len);
	if (res != ZCBOR_SUCCESS) {
		LOG_ERR("Could not encode keymap, %d", res);
		return;
	}

	/* Now that we know size of keymap report, encode and send a
	 * command response before we write the keymap
	 */
	cmd._command_len = write_len;
	res = cbor_encode_command(settings_write_buf + write_len,
			sizeof(settings_write_buf) - write_len,
			&cmd, &cmd_write_len);
	if (res != ZCBOR_SUCCESS) {
		LOG_ERR("Could not encode command, %d", res);
		return;
	}
	/* Write command response first (which is located later in buffer */
	res = settings_transport_write(settings_write_buf + write_len, cmd_write_len);
	if (res < 0) {
		LOG_ERR("Could not write to settings transport: %d", res);
		return;
	}
	res = settings_transport_write(settings_write_buf, write_len);
	if (res < 0) {
		LOG_ERR("Could not write to settings transport: %d", res);
		return;
	}
}

/*
 * This work handler incrementally reads data from the settings
 * transport backend.
 * Once a full CBOR indefinite list has been read in, the handler parses it as
 * a command structure. If a valid command structure is sent, the handler
 * will respond to the request. Otherwise it will reset the read buffer state
 * and start listening for new commands.
 */
static void zmk_settings_work_handler(struct k_work *work) {
	struct command cmd;
	int res;
	uint8_t *dst = (settings_read_buf + settings_read_offset);
	size_t payload_len;

	/* Read from input, see if we have received a full command */
	res = settings_transport_read(dst, SETTINGS_CMD_SIZE - settings_read_offset);
	if (res <= 0) {
		LOG_INF("No command read from transport, reset state");
		/* Reset input start, flush transport */
		settings_read_offset = 0;
		while (settings_transport_read(dst, 1)) {
			/* Flush data */
		}
		return;
	}
	/* Advance read offset */
	settings_read_offset += res;
	/* Try to parse command */
	res = cbor_decode_command(settings_read_buf,
				settings_read_offset,
				&cmd, &payload_len);
	if (res != ZCBOR_SUCCESS) {
		/* No valid command yet */
		return;
	}
	/* Valid command read, reset settings read buffer location */
	settings_read_offset = 0;
	switch (cmd._command_type_choice) {
		case _command_type_read_keymap:
			if (cmd._command_len != 0) {
				/* Command should not have data present, bail */
				return;
			}
			/* Respond with keymap */
			settings_read_keymap();
			break;
		case _command_type_set_keymap:
			__fallthrough;
		default:
			LOG_ERR("Not implemented!");
			return;
	}
}

void zmk_settings_init(void) {
	/* Setup settings work queue */
	k_work_init(&zmk_settings_work, zmk_settings_work_handler);
	/* Start transport backend */
	settings_transport_init();
}
