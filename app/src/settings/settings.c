/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/events/usb_feature_report.h>
#include <zmk/matrix.h>
#include <zmk/keymap.h>
#include <zmk/hid.h>
#include <zmk/config.h>
#include <zephyr/settings/settings.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

const struct zmk_hid_vendor_functions_report functions_report = {
    .report_id = SETTINGS_REPORT_ID_FUNCTIONS,
    .body = {
        .keycount = ZMK_KEYMAP_LEN,
	.layers = CONFIG_ZMK_SETTINGS_KEYMAP_LAYERS,
	.protocol_rev = 0x0,
        .key_remap_support = 0x1,
    },
};

uint32_t active_layer;
uint32_t active_key;
struct zmk_hid_vendor_key_data_report key_report = {
    .report_id = SETTINGS_REPORT_ID_KEY_DATA,
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

static int handle_keyboard_key_sel_report(const struct zmk_usb_feature_report *ev) {
    struct zmk_hid_vendor_key_sel_report *report;

    if (ev->direction == USB_REPORT_SET) {
        report = (struct zmk_hid_vendor_key_sel_report *)*ev->data;
        active_layer = report->body.layer_index;
        active_key = report->body.key_index;
        return ZMK_EV_EVENT_HANDLED;
    }
    /* Only SET direction is supported here */
    return -ENOTSUP;
}

static int handle_keyboard_key_data_report(const struct zmk_usb_feature_report *ev) {
    struct zmk_keymap_record record;
    int ret;

    if (ev->direction == USB_REPORT_GET) {
        ret = zmk_config_get_key_record(active_layer, active_key, &record);
        if (ret < 0) {
            return ret;
        }
        key_report.body.behavior_id = record.behavior_id;
        key_report.body.param1 = record.param1;
        key_report.body.param2 = record.param2;
        *ev->data = (uint8_t *)&key_report;
        *ev->len = sizeof(key_report);
        return ZMK_EV_EVENT_HANDLED;
    } else if (ev->direction == USB_REPORT_SET) {
        struct zmk_hid_vendor_key_data_report *set_report =
            (struct zmk_hid_vendor_key_data_report *)*ev->data;
        record.behavior_id = set_report->body.behavior_id;
        record.param1 = set_report->body.param1;
        record.param2 = set_report->body.param2;
        ret = zmk_config_set_key_record(active_layer, active_key, &record);
        if (ret < 0) {
            return ret;
        }
        return ZMK_EV_EVENT_HANDLED;
    }
    /* Other directions not supported */
    return -ENOTSUP;
}

static int handle_keyboard_key_commit(const struct zmk_usb_feature_report *ev) {
    int ret;

    if (ev->direction == USB_REPORT_SET) {
        ret = zmk_config_save_key_records();
        if (ret < 0) {
            return ret;
        }
        return ZMK_EV_EVENT_HANDLED;
    }
    /* Only SET direction supported */
    return -ENOTSUP;
}

static int feature_report_listener(const zmk_event_t *eh) {
    const struct zmk_usb_feature_report *ev = as_zmk_usb_feature_report(eh);
    switch (ev->id) {
    case SETTINGS_REPORT_ID_FUNCTIONS:
        return handle_keyboard_function_report(ev);
    case SETTINGS_REPORT_ID_KEY_SEL:
        return handle_keyboard_key_sel_report(ev);
    case SETTINGS_REPORT_ID_KEY_DATA:
        return handle_keyboard_key_data_report(ev);
    case SETTINGS_REPORT_ID_KEY_COMMIT:
        return handle_keyboard_key_commit(ev);
    default:
        return ZMK_EV_EVENT_BUBBLE;
    }
}

ZMK_LISTENER(settings, feature_report_listener);
ZMK_SUBSCRIPTION(settings, zmk_usb_feature_report);
