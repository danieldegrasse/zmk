/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zephyr/usb/class/hid.h>

/*
 * USB report direction. Set reports are sending data from the host to
 * device, and Get reports are requesting data from the device.
 */
enum usb_report_dir {
    USB_REPORT_SET,
    USB_REPORT_GET,
};

struct zmk_usb_feature_report {
    /* Report Direction */
    enum usb_report_dir direction;
    /* Report ID */
    uint8_t id;
    /*
     * Report data. For SET reports, will contain data for the device to read.
     * for GET reports, the event handler should set this pointer to an output
     * buffer with response data.
     */
    uint8_t **data;
    /*
     * Report length. For SET reports, will contain length of data for the
     * device to read. for GET reports, the event handler should set this
     * pointer to the length of the output buffer.
     */
    int32_t *len;
};

ZMK_EVENT_DECLARE(zmk_usb_feature_report);
