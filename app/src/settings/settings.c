/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/events/usb_feature_report.h>
#include <zmk/matrix.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

const struct zmk_hid_vendor_functions_report functions_report = {
    .report_id = SETTINGS_REPORT_ID_FUNCTIONS,
    .body = {
        .keycount = ZMK_KEYMAP_LEN,
	.protocol_rev = 0x0,
        .key_remap_support = 0x0,
    },
};

static int handle_keyboard_function_report(const struct zmk_usb_feature_report *ev) {
    if (ev->direction == USB_REPORT_GET) {
        *ev->data = (uint8_t *)&functions_report;
        *ev->len = sizeof(functions_report);
        return ZMK_EV_EVENT_HANDLED;
    }
    /* Only GET direction is supported here */
    return -ENOTSUP;
}

static int feature_report_listener(const zmk_event_t *eh) {
    const struct zmk_usb_feature_report *ev = as_zmk_usb_feature_report(eh);
    switch (ev->id) {
    case SETTINGS_REPORT_ID_FUNCTIONS:
        return handle_keyboard_function_report(ev);
    default:
        return ZMK_EV_EVENT_BUBBLE;
    }
}

ZMK_LISTENER(settings, feature_report_listener);
ZMK_SUBSCRIPTION(settings, zmk_usb_feature_report);
