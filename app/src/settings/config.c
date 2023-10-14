/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zmk/matrix.h>
#include <zmk/config.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

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

/* Partition name to use for NVS filesystem. Full partition will be used. */
#define NVS_PARTITION		storage_partition
#define NVS_PARTITION_SIZE      FIXED_PARTITION_SIZE(NVS_PARTITION)

/* NVS filesystem for keymap data */
static struct nvs_fs config_fs = {
    .flash_device = FIXED_PARTITION_DEVICE(NVS_PARTITION),
    .offset = FIXED_PARTITION_OFFSET(NVS_PARTITION),
};

/* Root identifiers for configuration types in NVS FS */
#define CONFIG_TYPE_KEY (0xC << 12)

/* Helper to map layer and key IDX to NVS ID */
#define CONFIG_KEY_RECORD(layer, key_off) \
    (CONFIG_TYPE_KEY | ((layer & 0x3F) << 8) | (key_off & 0xFF))

/* Array in RAM describing the current keymap state */
extern struct zmk_behavior_binding
	zmk_keymap[CONFIG_ZMK_SETTINGS_KEYMAP_LAYERS][ZMK_KEYMAP_LEN];

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

/* Loads all keymap settings from flash into RAM array */
static int zmk_config_load(void)
{
    struct zmk_behavior_binding tmp;
    int rc, id;
    uint8_t layer, key_off;

    /* Read the first keymap entry into a temporary buffer, and verify
     * it maps to a valid keymap. If not, clear the settings store.
     */
    rc = nvs_read(&config_fs, CONFIG_KEY_RECORD(0, 0), &tmp, sizeof(tmp));
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
        /* Clear all key entries in flash */
        for (layer = 0; layer < CONFIG_ZMK_SETTINGS_KEYMAP_LAYERS; layer++) {
            for (key_off = 0; key_off < ZMK_KEYMAP_LEN; key_off++) {
                rc = nvs_delete(&config_fs, CONFIG_KEY_RECORD(layer, key_off));
                if (rc < 0) {
                    LOG_ERR("Could not delete all keymap entries (%d)", rc);
                    return rc;
                }
            }
        }
    }

    /* We have valid keymap data. Read keymap data from NVS */
    for (layer = 0; layer < CONFIG_ZMK_SETTINGS_KEYMAP_LAYERS; layer++) {
        for (key_off = 0; key_off < ZMK_KEYMAP_LEN; key_off++) {
            rc = nvs_read(&config_fs, CONFIG_KEY_RECORD(layer, key_off),
                    &zmk_keymap[layer][key_off], sizeof(zmk_keymap[0][0]));
            if (rc != sizeof(zmk_keymap[0][0])) {
                LOG_ERR("Could not read key at [%d, %d] (%d)",
                        layer, key_off, rc);
                return rc;
            }
        }
    }
    return 0;
}

/* Initialization function called from main() to setup config subsystem */
int zmk_config_init(void) {
    int ret;
    uint32_t id;
    struct flash_pages_info info;

    /* NVS filesystem is defined to occupy entire storage region */
    if (!device_is_ready(config_fs.flash_device)) {
        LOG_ERR("Flash device %s not ready", config_fs.flash_device->name);
        return -ENODEV;
    }
    ret = flash_get_page_info_by_offs(config_fs.flash_device,
            config_fs.offset, &info);
    if (ret < 0) {
        LOG_ERR("Unable to get flash page information (%d)", ret);
        return ret;
    }
    config_fs.sector_size = info.size;
    config_fs.sector_count = NVS_PARTITION_SIZE / config_fs.sector_size;
    ret = nvs_mount(&config_fs);
    if (ret < 0) {
        LOG_ERR("Could not mount config filesystem (%d)", ret);
        return ret;
    }

    /* Now that the filesystem is loaded, read in all key data */
    ret = zmk_config_load();
    if (ret < 0) {
        LOG_ERR("Could not load ZMK config data (%d)", ret);
        return ret;
    }

    /* When all settings are loaded, we should make sure that the
     * string pointers within behavior bindings are replaced with pointers
     * into the constant string array we declared in this build.
     */
    for (uint8_t i = 0; i < CONFIG_ZMK_SETTINGS_KEYMAP_LAYERS; i++) {
        for (uint8_t j = 0; j < ZMK_KEYMAP_LEN; j++) {
            for (id = 0; id < ARRAY_SIZE(behavior_map); id++) {
                if (strcmp(zmk_keymap[i][j].behavior_dev, behavior_map[id]) == 0) {
                    if (zmk_keymap[i][j].behavior_dev != behavior_map[id]) {
                        /* Set new pointer, store it back to NVS */
                        zmk_keymap[i][j].behavior_dev = behavior_map[id];
                        ret = nvs_write(&config_fs, CONFIG_KEY_RECORD(i, j),
                                &zmk_keymap[i][j], sizeof(zmk_keymap[0][0]));
                        if (ret != sizeof(zmk_keymap[0][0])) {
                            LOG_ERR("Cannot write to NVS fs (%d)", ret);
                            return ret;
                        }
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
    /* Convert record to binding */
    return keymap_record_to_binding(&zmk_keymap[layer_index][key_index], record);
}

int zmk_config_save_key_records(void)
{
    int rc;
    uint8_t layer, key_off;

    /* Save all key records using NVS */
    for (layer = 0; layer < CONFIG_ZMK_SETTINGS_KEYMAP_LAYERS; layer++) {
        for (key_off = 0; key_off < ZMK_KEYMAP_LEN; key_off++) {
            rc = nvs_write(&config_fs, CONFIG_KEY_RECORD(layer, key_off),
                    &zmk_keymap[layer][key_off], sizeof(zmk_keymap[0][0]));
            /* Return code of 0 is fine here, indicates key was not changed */
            if (rc < 0) {
                return rc;
            }
        }
    }
    return 0;
}

