/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>

#include <zmk/usb.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#include <zmk/event_manager.h>
#include <zmk/events/usb_feature_report.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static const struct device *hid_dev;

static K_SEM_DEFINE(hid_sem, 1, 1);

static void in_ready_cb(const struct device *dev) { k_sem_give(&hid_sem); }

#ifdef CONFIG_USB_FEATURE_REPORTS

static int raise_report_event(bool get_report, struct usb_setup_packet *setup,
                              int32_t *len, uint8_t **data)
{
    int ret;
    uint8_t report_id = setup->wValue & 0xFF; /* Low 8 bytes have report ID */
    enum usb_report_dir dir = get_report ? USB_REPORT_GET : USB_REPORT_SET;
    /* Check the high byte of wValue to see if this was a feature report */
    if ((setup->wValue >> 8) != HID_FEATURE_REPORT_TYPE) {
        /* Not supported */
        return -ENOTSUP;
    }
    /* Raise a new feature report event */
    ret = ZMK_EVENT_RAISE(new_zmk_usb_feature_report(
        (struct zmk_usb_feature_report) {
            .direction = dir,
            .id = report_id,
            .data = data,
            .len = len,
        }));
    /*
     * Note- one issue with using the event handler is we cannot verify
     * that the event was actually handled, so we will respond to any
     * feature report from the host, even if there is not a handler
     * for that report ID.
     */
    if (ret) {
        LOG_DBG("Feature event handle returned an error: %d", ret);
        return -ENOTSUP;
    }
    return 0;
}


static int get_report_cb(const struct device *dev,
                    struct usb_setup_packet *setup,
                    int32_t *len, uint8_t **data)
{
    return raise_report_event(true, setup, len, data);
}

static int set_report_cb(const struct device *dev,
                    struct usb_setup_packet *setup,
                    int32_t *len, uint8_t **data)
{
    return raise_report_event(false, setup, len, data);
}

#endif /* CONFIG_USB_FEATURE_REPORTS */

static const struct hid_ops ops = {
    .int_in_ready = in_ready_cb,
#ifdef CONFIG_USB_FEATURE_REPORTS
    .get_report = get_report_cb,
    .set_report = set_report_cb,
#endif
};

int zmk_usb_hid_send_report(const uint8_t *report, size_t len) {
    switch (zmk_usb_get_status()) {
    case USB_DC_SUSPEND:
        return usb_wakeup_request();
    case USB_DC_ERROR:
    case USB_DC_RESET:
    case USB_DC_DISCONNECTED:
    case USB_DC_UNKNOWN:
        return -ENODEV;
    default:
        k_sem_take(&hid_sem, K_MSEC(30));
        int err = hid_int_ep_write(hid_dev, report, len, NULL);

        if (err) {
            k_sem_give(&hid_sem);
        }

        return err;
    }
}

static int zmk_usb_hid_init(const struct device *_arg) {
    hid_dev = device_get_binding("HID_0");
    if (hid_dev == NULL) {
        LOG_ERR("Unable to locate HID device");
        return -EINVAL;
    }

    usb_hid_register_device(hid_dev, zmk_hid_report_desc, sizeof(zmk_hid_report_desc), &ops);
    usb_hid_init(hid_dev);

    return 0;
}

SYS_INIT(zmk_usb_hid_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
