/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>

#include <zmk/keys.h>
#include <zmk/matrix.h>
#include <dt-bindings/zmk/hid_usage.h>
#include <dt-bindings/zmk/hid_usage_pages.h>

#define ZMK_HID_KEYBOARD_NKRO_MAX_USAGE HID_USAGE_KEY_KEYPAD_EQUAL

#define COLLECTION_REPORT 0x03

#define HID_USAGE_PAGE16(page)		\
    HID_ITEM(HID_ITEM_TAG_USAGE_PAGE, HID_ITEM_TYPE_GLOBAL, 2), \
    (page & 0xFF), ((page & 0xFF00) >> 8)

#define HID_INPUT_REPORT_TYPE 0x1
#define HID_OUTPUT_REPORT_TYPE 0x2
#define HID_FEATURE_REPORT_TYPE 0x3

#define GEN_DESKTOP_REPORT_ID 0x3
#define SETTINGS_REPORT_ID_FUNCTIONS 0x4
#define SETTINGS_REPORT_ID_KEY_SEL 0x5
#define SETTINGS_REPORT_ID_KEY_DATA 0x6
#define SETTINGS_REPORT_ID_KEY_COMMIT 0x7

static const uint8_t zmk_hid_report_desc[] = {
    HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
    HID_USAGE(HID_USAGE_GD_KEYBOARD),
    HID_COLLECTION(HID_COLLECTION_APPLICATION),
    HID_REPORT_ID(0x01),
    HID_USAGE_PAGE(HID_USAGE_KEY),
    HID_USAGE_MIN8(HID_USAGE_KEY_KEYBOARD_LEFTCONTROL),
    HID_USAGE_MAX8(HID_USAGE_KEY_KEYBOARD_RIGHT_GUI),
    HID_LOGICAL_MIN8(0x00),
    HID_LOGICAL_MAX8(0x01),

    HID_REPORT_SIZE(0x01),
    HID_REPORT_COUNT(0x08),
    /* INPUT (Data,Var,Abs) */
    HID_INPUT(0x02),

    HID_USAGE_PAGE(HID_USAGE_KEY),
    HID_REPORT_SIZE(0x08),
    HID_REPORT_COUNT(0x01),
    /* INPUT (Cnst,Var,Abs) */
    HID_INPUT(0x03),

    HID_USAGE_PAGE(HID_USAGE_KEY),

#if IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_NKRO)
    HID_LOGICAL_MIN8(0x00),
    HID_LOGICAL_MAX8(0x01),
    HID_USAGE_MIN8(0x00),
    HID_USAGE_MAX8(ZMK_HID_KEYBOARD_NKRO_MAX_USAGE),
    HID_REPORT_SIZE(0x01),
    HID_REPORT_COUNT(ZMK_HID_KEYBOARD_NKRO_MAX_USAGE + 1),
    /* INPUT (Data,Ary,Abs) */
    HID_INPUT(0x02),
#elif IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_HKRO)
    HID_LOGICAL_MIN8(0x00),
    HID_LOGICAL_MAX16(0xFF, 0x00),
    HID_USAGE_MIN8(0x00),
    HID_USAGE_MAX8(0xFF),
    HID_REPORT_SIZE(0x08),
    HID_REPORT_COUNT(CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE),
    /* INPUT (Data,Ary,Abs) */
    HID_INPUT(0x00),
#else
#error "A proper HID report type must be selected"
#endif

    HID_END_COLLECTION,
    HID_USAGE_PAGE(HID_USAGE_CONSUMER),
    HID_USAGE(HID_USAGE_CONSUMER_CONSUMER_CONTROL),
    HID_COLLECTION(HID_COLLECTION_APPLICATION),
    HID_REPORT_ID(0x02),
    HID_USAGE_PAGE(HID_USAGE_CONSUMER),

#if IS_ENABLED(CONFIG_ZMK_HID_CONSUMER_REPORT_USAGES_BASIC)
    HID_LOGICAL_MIN8(0x00),
    HID_LOGICAL_MAX16(0xFF, 0x00),
    HID_USAGE_MIN8(0x00),
    HID_USAGE_MAX8(0xFF),
    HID_REPORT_SIZE(0x08),
#elif IS_ENABLED(CONFIG_ZMK_HID_CONSUMER_REPORT_USAGES_FULL)
    HID_LOGICAL_MIN8(0x00),
    HID_LOGICAL_MAX16(0xFF, 0x0F),
    HID_USAGE_MIN8(0x00),
    HID_USAGE_MAX16(0xFF, 0x0F),
    HID_REPORT_SIZE(0x10),
#else
#error "A proper consumer HID report usage range must be selected"
#endif
    HID_REPORT_COUNT(CONFIG_ZMK_HID_CONSUMER_REPORT_SIZE),
    /* INPUT (Data,Ary,Abs) */
    HID_INPUT(0x00),
    HID_END_COLLECTION,
    HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
    HID_USAGE(HID_USAGE_GD_SYSTEM_CONTROL),
    HID_COLLECTION(HID_COLLECTION_APPLICATION),
    HID_REPORT_ID(GEN_DESKTOP_REPORT_ID),
    HID_LOGICAL_MIN8(0x00),
    HID_LOGICAL_MAX8(0x01),
    /* Support from power down usage (0x81) to menu exit usage (0x88) */
    HID_USAGE_MIN8(HID_USAGE_GD_SYSTEM_POWER_DOWN),
    HID_USAGE_MAX8(HID_USAGE_GD_SYSTEM_MENU_EXIT),
    HID_REPORT_SIZE(0x01),
    /* 8 usages- each usage is a single bit indicating if the usage is set */
    HID_REPORT_COUNT(0x08),
    /* INPUT report (Data,Var,Abs) */
    HID_INPUT(0x02),
    HID_END_COLLECTION,
#if IS_ENABLED(CONFIG_ZMK_SETTINGS)
    HID_USAGE_PAGE16(HID_USAGE_VENDOR),
    HID_USAGE(HID_USAGE_ZMK_KEYMAP),
    HID_COLLECTION(HID_COLLECTION_APPLICATION),
    HID_USAGE(HID_USAGE_ZMK_KEYMAP),
    HID_REPORT_ID(SETTINGS_REPORT_ID_FUNCTIONS),
    HID_LOGICAL_MIN8(0x00),
    HID_LOGICAL_MAX16(0xFF, 0x00),
    HID_REPORT_SIZE(0x28),
    HID_REPORT_COUNT(1),
    /* Feature (Data,Var,Abs) */
    HID_FEATURE(0x2),
    HID_USAGE(HID_USAGE_ZMK_KEYMAP),
    HID_REPORT_ID(SETTINGS_REPORT_ID_KEY_SEL),
    HID_REPORT_SIZE(0x10),
    HID_REPORT_COUNT(0x1),
    /* Feature (Data,Var,Abs) */
    HID_FEATURE(0x2),
    HID_USAGE(HID_USAGE_ZMK_KEYMAP),
    HID_REPORT_ID(SETTINGS_REPORT_ID_KEY_DATA),
    HID_REPORT_SIZE(0x60),
    HID_REPORT_COUNT(0x1),
    /* Feature (Data,Var,Abs) */
    HID_FEATURE(0x2),
    HID_USAGE(HID_USAGE_ZMK_KEYMAP),
    HID_REPORT_ID(SETTINGS_REPORT_ID_KEY_COMMIT),
    HID_REPORT_SIZE(0x00),
    HID_REPORT_COUNT(0x0),
    /* Feature (Data,Var,Abs) */
    HID_FEATURE(0x2),
    HID_END_COLLECTION,
#endif /* CONFIG_ZMK_SETTINGS */
};

// struct zmk_hid_boot_report
// {
//     uint8_t modifiers;
//     uint8_t _unused;
//     uint8_t keys[6];
// } __packed;

struct zmk_hid_keyboard_report_body {
    zmk_mod_flags_t modifiers;
    uint8_t _reserved;
#if IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_NKRO)
    uint8_t keys[(ZMK_HID_KEYBOARD_NKRO_MAX_USAGE + 1) / 8];
#elif IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_HKRO)
    uint8_t keys[CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE];
#endif
} __packed;

struct zmk_hid_keyboard_report {
    uint8_t report_id;
    struct zmk_hid_keyboard_report_body body;
} __packed;

struct zmk_hid_consumer_report_body {
#if IS_ENABLED(CONFIG_ZMK_HID_CONSUMER_REPORT_USAGES_BASIC)
    uint8_t keys[CONFIG_ZMK_HID_CONSUMER_REPORT_SIZE];
#elif IS_ENABLED(CONFIG_ZMK_HID_CONSUMER_REPORT_USAGES_FULL)
    uint16_t keys[CONFIG_ZMK_HID_CONSUMER_REPORT_SIZE];
#endif
} __packed;

struct zmk_hid_consumer_report {
    uint8_t report_id;
    struct zmk_hid_consumer_report_body body;
} __packed;

struct zmk_hid_gen_desktop_report_body {
    uint8_t keys; /* Bitmask of usage IDs that are pressed */
} __packed;

struct zmk_hid_gen_desktop_report {
    uint8_t report_id;
    struct zmk_hid_gen_desktop_report_body body;
} __packed;

#if IS_ENABLED(CONFIG_SETTINGS)

struct zmk_hid_vendor_functions_report_body {
    /* Number of keys on this keyboard */
    uint8_t keycount;
    /* Number of layers supported/used by this keyboard */
    uint8_t layers;
    /* Protocol revision */
    uint8_t protocol_rev;
    /* Flag to indicate dynamic keymap support */
    uint8_t key_remap_support : 1;
    /* Flags reserved for future functionality */
    uint8_t reserved : 7;
} __packed;

struct zmk_hid_vendor_functions_report {
    uint8_t report_id;
    struct zmk_hid_vendor_functions_report_body body;
} __packed;


struct zmk_hid_vendor_key_sel_report_body {
    uint8_t layer_index;
    uint8_t key_index;
} __packed;

struct zmk_hid_vendor_key_sel_report {
    uint8_t report_id;
    struct zmk_hid_vendor_key_sel_report_body body;
} __packed;


struct zmk_hid_vendor_key_data_report_body {
    uint32_t behavior_id;
    uint32_t param1;
    uint32_t param2;
} __packed;

struct zmk_hid_vendor_key_data_report {
    uint8_t report_id;
    struct zmk_hid_vendor_key_data_report_body body;
} __packed;

struct zmk_hid_vendor_key_commit_report {
    uint8_t report_id;
} __packed;

#endif /* CONFIG_SETTINGS */

zmk_mod_flags_t zmk_hid_get_explicit_mods();
int zmk_hid_register_mod(zmk_mod_t modifier);
int zmk_hid_unregister_mod(zmk_mod_t modifier);
bool zmk_hid_mod_is_pressed(zmk_mod_t modifier);

int zmk_hid_register_mods(zmk_mod_flags_t explicit_modifiers);
int zmk_hid_unregister_mods(zmk_mod_flags_t explicit_modifiers);
int zmk_hid_implicit_modifiers_press(zmk_mod_flags_t implicit_modifiers);
int zmk_hid_implicit_modifiers_release();
int zmk_hid_masked_modifiers_set(zmk_mod_flags_t masked_modifiers);
int zmk_hid_masked_modifiers_clear();

int zmk_hid_keyboard_press(zmk_key_t key);
int zmk_hid_keyboard_release(zmk_key_t key);
void zmk_hid_keyboard_clear();
bool zmk_hid_keyboard_is_pressed(zmk_key_t key);

int zmk_hid_consumer_press(zmk_key_t key);
int zmk_hid_consumer_release(zmk_key_t key);
void zmk_hid_consumer_clear();
bool zmk_hid_consumer_is_pressed(zmk_key_t key);

int zmk_hid_press(uint32_t usage);
int zmk_hid_release(uint32_t usage);
bool zmk_hid_is_pressed(uint32_t usage);

struct zmk_hid_keyboard_report *zmk_hid_get_keyboard_report();
struct zmk_hid_consumer_report *zmk_hid_get_consumer_report();
struct zmk_hid_gen_desktop_report *zmk_hid_get_gen_desktop_report();
