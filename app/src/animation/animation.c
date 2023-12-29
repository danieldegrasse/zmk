/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_animation

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <math.h>

#include <drivers/animation.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/led.h>

#include <zmk/animation.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define PHANDLE_TO_DEVICE(node_id, prop, idx) DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

#define PHANDLE_TO_CHAIN_LENGTH(node_id, prop, idx)                                                \
    DT_PROP_BY_PHANDLE_IDX(node_id, prop, idx, chain_length),

#define PHANDLE_TO_PIXEL(node_id, prop, idx)                                                       \
    {                                                                                              \
        .position_x = DT_PHA_BY_IDX(node_id, prop, idx, position_x),                               \
        .position_y = DT_PHA_BY_IDX(node_id, prop, idx, position_y),                               \
    },

/**
 * LED Driver device pointers.
 */
static const struct device *drivers[] = {
    DT_INST_FOREACH_PROP_ELEM(0, drivers, PHANDLE_TO_DEVICE)
};

/**
 * Size of the LED driver device pointers array.
 */
static const size_t drivers_size = DT_INST_PROP_LEN(0, drivers);

/**
 * Array containing the number of LEDs handled by each device.
 */
#if DT_INST_NODE_HAS_PROP(0, chain_lengths)
static const uint8_t pixels_per_driver[] = DT_INST_PROP(0, chain_lengths);
#else
static const uint8_t pixels_per_driver[] = {
    DT_INST_FOREACH_PROP_ELEM(0, drivers, PHANDLE_TO_CHAIN_LENGTH)
};
#endif

static const struct device *animation_root = DEVICE_DT_GET(DT_CHOSEN(zmk_animation));

/**
 * Pixel configuration.
 */
static struct animation_pixel pixels[] = {
    DT_INST_FOREACH_PROP_ELEM(0, pixels, PHANDLE_TO_PIXEL)
};

/**
 * Size of the pixels array.
 */
static const size_t pixels_size = DT_INST_PROP_LEN(0, pixels);


#if DT_INST_NODE_HAS_PROP(0, chain_reserved_ranges)
#define CHAIN_HAS_RESERVED_RANGES 1
/*
 * Expands to 0 if property index is even, or property value if it is odd
 */
#define ODD_ARRAY_PROP(node_id, prop, idx) \
    (((idx & 0x1) == 0) ? 0 : DT_PROP_BY_IDX(node_id, prop, idx))
/*
 * Macro to get true size of pixel buffer, based on reserved ranges
 * set by `chain-reserved-ranges`. These ranges should be present as
 * zeroed values in the final buffer sent to the controller
 */
#define PIXEL_BUF_SIZE \
    DT_INST_PROP_LEN(0, pixels) + \
    (DT_INST_FOREACH_PROP_ELEM_SEP(0, chain_reserved_ranges, ODD_ARRAY_PROP, (+)))

/* Array mapping pixel indices to offsets in pixel buffer */
static uint8_t px_buffer_indices[DT_INST_PROP_LEN(0, pixels)];

static const uint8_t raw_offsets[] = DT_INST_PROP(0, chain_reserved_ranges);

/* Structure describing offsets of pixels within px_buffer */
struct px_buffer_offset {
    uint8_t offset;
    uint8_t len;
};

struct px_buffer_offset *px_offsets = (struct px_buffer_offset *)raw_offsets;

#else
#define PIXEL_BUF_SIZE DT_INST_PROP_LEN(0, pixels)
#endif

/**
 * Buffer for RGB values ready to be sent to the drivers.
 */
#ifdef CONFIG_ZMK_ANIMATION_TYPE_RGB
static struct led_rgb px_buffer[PIXEL_BUF_SIZE];
#elif defined(CONFIG_ZMK_ANIMATION_TYPE_MONO)
static uint8_t px_buffer[PIXEL_BUF_SIZE];
#endif

/**
 * Counter for animation frames that have been requested but have yet to be executed.
 */
static uint32_t animation_timer_countdown = 0;

/**
 * Conditional implementation of zmk_animation_get_pixel_by_key_position
 * if key-pixels is set.
 */
#if DT_INST_NODE_HAS_PROP(0, key_pixels)
static const uint8_t pixels_by_key_position[] = DT_INST_PROP(0, key_pixels);

size_t zmk_animation_get_pixel_by_key_position(size_t key_position) {
    return pixels_by_key_position[key_position];
}
#endif

#if defined(CONFIG_ZMK_ANIMATION_PIXEL_DISTANCE) && (CONFIG_ZMK_ANIMATION_PIXEL_DISTANCE == 1)

/**
 * Lookup table for distance between any two pixels.
 *
 * The values are stored as a triangular matrix which cuts the space requirement roughly in half.
 */
static uint8_t
    pixel_distance[((DT_INST_PROP_LEN(0, pixels) + 1) * DT_INST_PROP_LEN(0, pixels)) / 2];

uint8_t zmk_animation_get_pixel_distance(size_t pixel_idx, size_t other_pixel_idx) {
    if (pixel_idx < other_pixel_idx) {
        return zmk_animation_get_pixel_distance(other_pixel_idx, pixel_idx);
    }

    return pixel_distance[(((pixel_idx + 1) * pixel_idx) >> 1) + other_pixel_idx];
}

#endif

#ifdef CONFIG_ZMK_ANIMATION_TYPE_RGB
static void zmk_animation_tick(struct k_work *work) {
    LOG_DBG("Animation tick");
    animation_render_frame(animation_root, &pixels[0], pixels_size);

    for (size_t i = 0; i < pixels_size; ++i) {
#if defined(CHAIN_HAS_RESERVED_RANGES)
        zmk_rgb_to_led_rgb(&pixels[i].value, &px_buffer[px_buffer_indices[i]]);
#else
        zmk_rgb_to_led_rgb(&pixels[i].value, &px_buffer[i]);
#endif


        // Reset values for the next cycle
        pixels[i].value.r = 0;
        pixels[i].value.g = 0;
        pixels[i].value.b = 0;
    }

    size_t pixels_updated = 0;

    for (size_t i = 0; i < drivers_size; ++i) {
        led_strip_update_rgb(drivers[i], &px_buffer[pixels_updated], pixels_per_driver[i]);

        pixels_updated += (size_t)pixels_per_driver[i];
    }
}

#elif defined(CONFIG_ZMK_ANIMATION_TYPE_MONO)

static void zmk_animation_tick(struct k_work *work) {
    LOG_DBG("Animation tick");
    animation_render_frame(animation_root, &pixels[0], pixels_size);

    for (size_t i = 0; i < pixels_size; ++i) {
#if defined(CHAIN_HAS_RESERVED_RANGES)
        zmk_rgb_to_mono(&pixels[i].value, &px_buffer[px_buffer_indices[i]]);
#else
        zmk_rgb_to_mono(&pixels[i].value, &px_buffer[i]);
#endif

        // Reset values for the next cycle
        pixels[i].value.r = 0;
        pixels[i].value.g = 0;
        pixels[i].value.b = 0;
    }

    size_t pixels_updated = 0;

    for (size_t i = 0; i < drivers_size; ++i) {
	led_write_channels(drivers[i], 0, pixels_per_driver[i], &px_buffer[pixels_updated]);

        pixels_updated += (size_t)pixels_per_driver[i];
    }
}

#endif

K_WORK_DEFINE(animation_work, zmk_animation_tick);

static void zmk_animation_tick_handler(struct k_timer *timer) {
    if (--animation_timer_countdown == 0) {
        k_timer_stop(timer);
    }

    k_work_submit(&animation_work);
}

K_TIMER_DEFINE(animation_tick, zmk_animation_tick_handler, NULL);

void zmk_animation_request_frames(uint32_t frames) {
    if (frames <= animation_timer_countdown) {
        return;
    }

    if (animation_timer_countdown == 0) {
        k_timer_start(&animation_tick, K_MSEC(1000 / CONFIG_ZMK_ANIMATION_FPS),
                      K_MSEC(1000 / CONFIG_ZMK_ANIMATION_FPS));
    }

    animation_timer_countdown = frames;
}

static int zmk_animation_on_activity_state_changed(const zmk_event_t *event) {
    const struct zmk_activity_state_changed *activity_state_event;

    if ((activity_state_event = as_zmk_activity_state_changed(event)) == NULL) {
        // Event not supported.
        return -ENOTSUP;
    }

    switch (activity_state_event->state) {
    case ZMK_ACTIVITY_ACTIVE:
        animation_start(animation_root);
        return 0;
    case ZMK_ACTIVITY_SLEEP:
        animation_stop(animation_root);
        k_timer_stop(&animation_tick);
        animation_timer_countdown = 0;
        return 0;
    default:
        return 0;
    }
}

static int zmk_animation_init(const struct device *dev) {
#if defined(CONFIG_ZMK_ANIMATION_PIXEL_DISTANCE) && (CONFIG_ZMK_ANIMATION_PIXEL_DISTANCE == 1)
    // Prefill the pixel distance lookup table
    int k = 0;
    for (size_t i = 0; i < pixels_size; ++i) {
        for (size_t j = 0; j <= i; ++j) {
            // Distances are normalized to fit inside 0-255 range to fit inside uint8_t
            // for better space efficiency
            pixel_distance[k++] = sqrt(pow(pixels[i].position_x - pixels[j].position_x, 2) +
                                       pow(pixels[i].position_y - pixels[j].position_y, 2)) *
		                  255 / 360;
        }
    }
#endif
#if defined(CHAIN_HAS_RESERVED_RANGES)
    /* Prefill the lookup table with pixel offsets */
    int pxbuf_idx = 0;
    int pixel_idx = 0;
    for (size_t i = 0; i < sizeof(raw_offsets) / 2; i++) {
        while (pxbuf_idx < (px_offsets[i].offset - 1)) {
            px_buffer_indices[pixel_idx++] = pxbuf_idx++;
        }
        pxbuf_idx += px_offsets[i].len;
    }
#endif

    LOG_INF("ZMK Animation Ready");

    animation_start(animation_root);

    return 0;
}

SYS_INIT(zmk_animation_init, APPLICATION, 95);

ZMK_LISTENER(amk_animation, zmk_animation_on_activity_state_changed);
ZMK_SUBSCRIPTION(amk_animation, zmk_activity_state_changed);
