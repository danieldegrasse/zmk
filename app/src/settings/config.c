/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zmk/matrix.h>
#include <zmk/config.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define BEHAVIOR_MAP(node_id)                                           \
    [DT_PROP(node_id, behavior_id)] = DT_PROP(node_id, label),

/*
 * This mapping relates behavior IDs to device labels, using
 * the behavior ID as an index.
 */
static const char *behavior_map[] = {
    DT_FOREACH_CHILD(DT_PATH(behaviors), BEHAVIOR_MAP)
};

/* Array in RAM describing the current keymap state */
extern struct zmk_behavior_binding zmk_keymap[CONFIG_ZMK_SETTINGS_KEYMAP_LAYERS][ZMK_KEYMAP_LEN];

/*
 * Tracks if keymap is dirty and need to be written
 * to flash.
 */
static bool keymap_dirty;

/*
 * Converts a keymap record to a keymap,
 * usable with keymap.c code.
 */
static int keymap_record_to_binding(struct zmk_behavior_binding *out,
                                    struct zmk_keymap_record *in) {
    if ((in->behavior_id >= ARRAY_SIZE(behavior_map)) ||
        (behavior_map[in->behavior_id] == NULL)) {
        return -EINVAL;
    }
    out->behavior_dev = (char *)behavior_map[in->behavior_id];
    out->param1 = in->param1;
    out->param2 = in->param2;
    return 0;
}

/*
 * Converts a keymap binding to a keymap record
 */
static int keymap_binding_to_record(struct zmk_keymap_record *out,
                                    struct zmk_behavior_binding *in) {
    uint32_t id;
    for (id = 0; id < ARRAY_SIZE(behavior_map); id++) {
        if (strcmp(in->behavior_dev, behavior_map[id]) == 0) {
            break;
        }
    }
    if (id == ARRAY_SIZE(behavior_map)) {
        return -EINVAL;
    }

    out->behavior_id = id;
    out->param1 = in->param1;
    out->param2 = in->param2;
    return 0;
}

static int keymap_settings_set(const char *name, size_t len,
                            settings_read_cb read_cb, void *cb_arg) {
    const char *next;
    int rc;
    uint8_t id;
    struct zmk_behavior_binding tmp;

    if (settings_name_steq(name, "keymap", &next) && !next) {
        /* Read the first keymap entry into a temporary buffer, and verify
         * it maps to a valid keymap. If not, clear the settings store.
         */
        rc = read_cb(cb_arg, &tmp, sizeof(tmp));
        if (rc != sizeof(tmp)) {
            return rc;
        }
        for (id = 0; id < ARRAY_SIZE(behavior_map); id++) {
            if (behavior_map[id] == tmp.behavior_dev) {
                break;
            }
        }
        if (id == ARRAY_SIZE(behavior_map)) {
            LOG_WRN("Keymap found in flash appears invalid, using default");
            return settings_delete("keys/keymap");
        }
        /* Read keymap data, read always starts at beginning of setting */
        rc = read_cb(cb_arg, &zmk_keymap, len);
        if (rc == len) {
            /* Expected length was read, return success */
            return 0;
        }
        return rc;
    }

    return -ENOENT;
}

static int keymap_settings_export(int (*storage_func)(const char *name,
                                    const void *value,
                                    size_t val_len)) {
    int rc;

    if (keymap_dirty) {
        rc = storage_func("keys/keymap", zmk_keymap, CONFIG_ZMK_SETTINGS_KEYMAP_LAYERS *
                          ZMK_KEYMAP_LEN * sizeof(struct zmk_behavior_binding));
        if (rc < 0) {
            return rc;
        }
    }
    return 0;
}

static int keymap_settings_commit(void) {
    return 0;
}

struct settings_handler keymap_conf = {
    .name = "keys",
    .h_set = keymap_settings_set,
    .h_export = keymap_settings_export,
    .h_commit = keymap_settings_commit,
};

/* Initialization function called from main() to setup config subsystem */
int zmk_config_init(void) {
    int ret;

    ret = settings_subsys_init();
    if (ret < 0) {
        LOG_ERR("Settings subsystem init failed: (%d)", ret);
        return ret;
    }

    ret = settings_register(&keymap_conf);
    if (ret < 0) {
        LOG_ERR("Keymap registration failed: (%d)", ret);
        return ret;
    }

    ret = settings_load();
    if (ret < 0) {
        LOG_ERR("Settings load failed: (%d)", ret);
        return ret;
    }


    /* When all settings are loaded, we should make sure that the
     * string pointers within behavior bindings are replaced with pointers
     * into the constant string array we declared in this build.
     */
    uint32_t id;

    for (uint8_t i = 0; i < CONFIG_ZMK_SETTINGS_KEYMAP_LAYERS; i++) {
        for (uint8_t j = 0; j < ZMK_KEYMAP_LEN; j++) {
            for (id = 0; id < ARRAY_SIZE(behavior_map); id++) {
                if (strcmp(zmk_keymap[i][j].behavior_dev, behavior_map[id]) == 0) {
                    if (zmk_keymap[i][j].behavior_dev != behavior_map[id]) {
                        /* Set new pointer, mark keymap as dirty */
                        zmk_keymap[i][j].behavior_dev = behavior_map[id];
                        keymap_dirty = true;
                    }
                    break;
                }
            }
            if (id == ARRAY_SIZE(behavior_map)) {
                return -EINVAL;
            }
        }
    }

    return 0;
}

int zmk_config_get_key_record(uint8_t layer_index, uint8_t key_index,
                                struct zmk_keymap_record *record)
{
    if (layer_index > CONFIG_ZMK_SETTINGS_KEYMAP_LAYERS) {
        return -EINVAL;
    }
    if (key_index > ZMK_KEYMAP_LEN) {
        return -EINVAL;
    }
    /* Convert binding to record */
    return keymap_binding_to_record(record, &zmk_keymap[layer_index][key_index]);
}

int zmk_config_set_key_record(uint8_t layer_index, uint8_t key_index,
                                struct zmk_keymap_record *record)
{
    if (layer_index > CONFIG_ZMK_SETTINGS_KEYMAP_LAYERS) {
        return -EINVAL;
    }
    if (key_index > ZMK_KEYMAP_LEN) {
        return -EINVAL;
    }
    keymap_dirty = true;
    /* Convert binding to record */
    return keymap_record_to_binding(&zmk_keymap[layer_index][key_index], record);
}
