/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_animation_indicator

#include <zephyr/device.h>

#include <zephyr/logging/log.h>

#include <drivers/animation.h>

#include <zmk/animation.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct animation_indicator_config {
    size_t *pixel_map;
    size_t pixel_map_size;
    uint16_t page; /* HID page of key to indicate status of */
    uint32_t id; /* HID ID of key to indicate status of */
    struct zmk_color_hsl *hsl; /* HSL color set in devicetree */
};

struct animation_indicator_data {
    bool active; /* Tracks if animation is active */
    bool pressed; /* Tracks pressed state of key */
    struct zmk_color_rgb rgb; /* RGB pixel value we will write to zone */
};

static int animation_indicator_on_key_press(const struct device *dev,
                                            const zmk_event_t *event)
{
    const struct animation_indicator_config *config = dev->config;
    struct animation_indicator_data *data = dev->data;
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(event);

    if (ev == NULL || !ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    /* Check to see if the key event matches our page and ID */
    if ((ev->usage_page == config->page) && (ev->keycode == config->id)) {
        data->pressed = !data->pressed;

        /* Request a frame to update zone */
        zmk_animation_request_frames(1);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

static void animation_indicator_start(const struct device *dev) {
    /* Request a frame */
    zmk_animation_request_frames(1);
}

static void animation_indicator_stop(const struct device *dev) {
    /* Nothing to do */
}

static int animation_indicator_init(const struct device *dev) {
    const struct animation_indicator_config *config = dev->config;
    struct animation_indicator_data *data = dev->data;

    zmk_hsl_to_rgb(config->hsl, &data->rgb);
    return 0;
}

static void animation_indicator_render_frame(const struct device *dev,
                                             struct animation_pixel *pixels,
                                             size_t num_pixels)
{
    const struct animation_indicator_config *config = dev->config;
    struct animation_indicator_data *data = dev->data;

    for (size_t i = 0; i < config->pixel_map_size; i++) {
        /* If key is pressed, set the color. Otherwise clear it */
        if (data->pressed) {
            pixels[config->pixel_map[i]].value = data->rgb;
        } else {
            memset(&pixels[config->pixel_map[i]].value, 0,
                   sizeof(pixels[config->pixel_map[i]].value));
        }
    }
}



static const struct animation_api animation_indicator_api = {
    .on_start = animation_indicator_start,
    .on_stop = animation_indicator_stop,
    .render_frame = animation_indicator_render_frame,
};

#define ANIMATION_INDICATOR_DEVICE(inst)                                            \
    static size_t animation_indicator_##inst##_pixel_map[] =                        \
                                        DT_INST_PROP(inst, pixels);                 \
    static uint32_t animation_indicator_##inst##_color = DT_INST_PROP(inst, color); \
    static const struct animation_indicator_config                                  \
        animation_indicator_cfg_##inst = {                                          \
            .pixel_map = animation_indicator_##inst##_pixel_map,                    \
            .pixel_map_size = DT_INST_PROP_LEN(inst, pixels),                       \
            .page = ZMK_HID_USAGE_PAGE(DT_INST_PROP(inst, key)),                    \
            .id = ZMK_HID_USAGE_ID(DT_INST_PROP(inst, key)),                        \
            .hsl = (struct zmk_color_hsl*)&animation_indicator_##inst##_color,      \
    };                                                                              \
    static struct animation_indicator_data                                          \
        animation_indicator_data_##inst;                                            \
    DEVICE_DT_INST_DEFINE(inst, animation_indicator_init, NULL,                     \
                          &animation_indicator_data_##inst,                         \
                          &animation_indicator_cfg_##inst,                          \
                          POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,            \
                          &animation_indicator_api);                                \
                                                                                    \
    static int animation_indicator_##inst##_event_handler(const zmk_event_t *event) \
    {                                                                               \
        const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(inst));                \
                                                                                    \
        return animation_indicator_on_key_press(dev, event);                        \
    }                                                                               \
                                                                                    \
    ZMK_LISTENER(animation_indicator_##inst,                                        \
                 animation_indicator_##inst##_event_handler);                       \
    ZMK_SUBSCRIPTION(animation_indicator_##inst, zmk_keycode_state_changed);

DT_INST_FOREACH_STATUS_OKAY(ANIMATION_INDICATOR_DEVICE);
