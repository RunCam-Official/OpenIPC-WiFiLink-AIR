#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "wifilink.h"
#include "configuration.h"
#include "dictionary.h"
#include "iniparser.h"
#include "conf_parser.h"
#include "led.h"
#include "debug.h"

#define MAX_LINES 1024
#define MAX_LINE_LEN 256

typedef enum {
    PARAM_TYPE_INT,
    PARAM_TYPE_STRING
} ParamType;

typedef struct {
    const char* section;
    const char* key;
    ParamType type;
    void* air_param_ptr;
    int min_val;
    int max_val;
} ConfigParam;

AirParam_s airParam;

static int handle_config_param(ConfigParam *param, dictionary *ini_dict, bool *modified);
static int handle_YAML_param(ConfigParam *param, dictionary *ini_dict, bool *modified);

static ConfigParam wfb_params[] = {
    {"wfb", "channel", PARAM_TYPE_INT, &airParam.wfb_channel, 0, 0},
    {"wfb", "driver_txpower_override", PARAM_TYPE_INT, &airParam.wfb_driver_txpower_override, 0, 0},
    {"wfb", "stbc", PARAM_TYPE_INT, &airParam.wfb_stbc, 0, 1},
    {"wfb", "ldpc", PARAM_TYPE_INT, &airParam.wfb_ldpc, 0, 1},
    {"wfb", "mcs_index", PARAM_TYPE_INT, &airParam.wfb_mcs_index, 0, 9}
};

static ConfigParam telemetry_params[] = {
    {"telemetry", "router", PARAM_TYPE_INT, &airParam.telemetry_router, 0, 0}
};

static ConfigParam majestic_params[] = {
    {"video0", "codec", PARAM_TYPE_STRING, airParam.maj_video_codec, 0, 0},
    {"video0", "size", PARAM_TYPE_STRING, airParam.maj_video_size, 0, 0},
    {"video0", "fps", PARAM_TYPE_INT, &airParam.maj_video_fps, 0, 0},
    {"video0", "bitrate", PARAM_TYPE_INT, &airParam.maj_video_bitrate, 4096, 19968},
    {"records", "enabled", PARAM_TYPE_STRING, airParam.maj_video_records, 0, 0},
    {"image", "mirror", PARAM_TYPE_STRING, airParam.maj_image_mirror, 0, 0},
    {"image", "flip", PARAM_TYPE_STRING, airParam.maj_image_flip, 0, 0},
    {"image", "rotate", PARAM_TYPE_INT, &airParam.maj_image_rotate, 0, 0},
    {"image", "contrast", PARAM_TYPE_INT, &airParam.maj_image_contrast, 0, 100},
    {"image", "hue", PARAM_TYPE_INT, &airParam.maj_image_hue, 0, 100},
    {"image", "saturation", PARAM_TYPE_INT, &airParam.maj_image_saturation, 0, 100},
    {"image", "luminance", PARAM_TYPE_INT, &airParam.maj_image_luminance, 0, 100},
    {"audio", "enabled", PARAM_TYPE_STRING, airParam.maj_audio_enable, 0, 0},
    {"audio", "volume", PARAM_TYPE_INT, &airParam.maj_audio_volume, 0, 30}
};

Config* load_config(const char *file_path) {
    Config *config = confparser_load(file_path);
    if (!config) {
        log_err("Failed to load %s\n", file_path);
    }
    return config;
}

void load_air_param(Config *wfb_config, Config *telemetry_config) {
    int i;
    for (i = 0; i < sizeof(wfb_params) / sizeof(wfb_params[0]); i++) {
        if (wfb_params[i].type == PARAM_TYPE_INT) {
            *(int *)wfb_params[i].air_param_ptr = confparser_getint(wfb_config, wfb_params[i].key);
            log_dump("%s:%s = %d", wfb_params[i].section, wfb_params[i].key, *(int *)wfb_params[i].air_param_ptr);
        }
    }

    for (i = 0; i < sizeof(telemetry_params) / sizeof(telemetry_params[0]); i++) {
        if (telemetry_params[i].type == PARAM_TYPE_INT) {
            *(int *)telemetry_params[i].air_param_ptr = confparser_getint(telemetry_config, telemetry_params[i].key);
            log_dump("%s:%s = %d", telemetry_params[i].section, telemetry_params[i].key, *(int *)telemetry_params[i].air_param_ptr);
        }
    }

    for (i = 0; i < sizeof(majestic_params) / sizeof(majestic_params[0]); i++) {
        if (majestic_params[i].type == PARAM_TYPE_INT) {
            wifilink_yaml_getint(YAML_CLI_GET, majestic_params[i].section, majestic_params[i].key, majestic_params[i].air_param_ptr);
            log_dump("%s:%s = %d", majestic_params[i].section, majestic_params[i].key, *(int *)majestic_params[i].air_param_ptr);
        } else {
            wifilink_yaml(YAML_CLI_GET, majestic_params[i].section, majestic_params[i].key, majestic_params[i].air_param_ptr);
            log_dump("%s:%s = %s", majestic_params[i].section, majestic_params[i].key, (char *)majestic_params[i].air_param_ptr);
        }
    }
}

int get_air_init_param(void) {
    Config *wfb_config = load_config(WFB_FILE_PATH);
    if (!wfb_config) return -1;

    Config *telemetry_config = load_config(TELEMETRY_FILE_PATH);
    if (!telemetry_config) {
        confparser_free(wfb_config);
        return -1;
    }

    memset(&airParam, 0, sizeof(airParam));
    load_air_param(wfb_config, telemetry_config);

    confparser_free(wfb_config);
    confparser_free(telemetry_config);

    return 0;
}

int compare(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

int DynamicallyModifyTxPower(int txPower)
{
    FILE *fp;
    char line[MAX_LINE_LEN];
    char *unique[MAX_LINES];
    int count = 0;

    fp = popen("lsusb", "r");
    if (fp == NULL) {
        log_err("Failed to run lsusb");
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *token = strtok(line, " ");
        int col = 1;
        while (token != NULL) {
            if (col == 6) {
                unique[count] = strdup(token); 
                count++;
                break;
            }
            token = strtok(NULL, " ");
            col++;
        }
    }
    pclose(fp);

    qsort(unique, count, sizeof(char *), compare);

    for (int i = 0; i < count; i++) {
        if (i == 0 || strcmp(unique[i], unique[i - 1]) != 0) {
            if(strcmp(unique[i], "0bda:a81a\n") == 0 || strcmp(unique[i], "0bda:f72b\n") == 0)
            {
                char cmdBuff[64] = {"0"};
                sprintf(cmdBuff, "iw dev wlan0 set txpower fixed %d", txPower*50);
                log_debug("cmdBuff: %s", cmdBuff);
                wifilink_system(cmdBuff);
            }
        }
        free(unique[i]);
    }
    return 0;
}

int CardConfigurationCheck(void)
{
    size_t i = 0;
    bool WfbModifyFlag = false;
    bool MajesticModifyFlag = false;
    bool TelemetryModifyFlag = false;

    dictionary *pstDict = NULL;

    pstDict = iniparser_load(USER_INI_FILE_PATH);
    if (pstDict == NULL) {
        log_err("Failed to load %s\n", USER_INI_FILE_PATH);
        return -1;
    }

    for (i = 0; i < sizeof(wfb_params)/sizeof(wfb_params[0]); i++)
        handle_config_param(&wfb_params[i], pstDict, &WfbModifyFlag);

    for (i = 0; i < sizeof(telemetry_params)/sizeof(telemetry_params[0]); i++)
        handle_config_param(&telemetry_params[i], pstDict, &TelemetryModifyFlag);


    for (i = 0; i < sizeof(majestic_params)/sizeof(majestic_params[0]); i++)
        handle_YAML_param(&majestic_params[i], pstDict, &MajesticModifyFlag);


    iniparser_freedict(pstDict);

    if(MajesticModifyFlag)
    {
        log_debug("Modify Majestic!");
        wifilink_system("killall -1 majestic");
        get_air_init_param();
    }
    
    if(WfbModifyFlag || TelemetryModifyFlag)
    {
        wifilink_system("reboot");
        return 0;
    }

    audio_state_led();
    records_state_led();
    
    return 0;
}

void print_dictionary(dictionary *dict) {
    log_debug();
    if (dict == NULL) {
        printf("Dictionary is NULL.\n");
        return;
    }

    log_debug("Number of entries: %d\n", dict->n);
    for (int i = 0; i < dict->n; i++) {
        log_debug("Key: %s, Value: %s\n", dict->key[i], dict->val[i]);
    }
}

int update_ini_file(const char *filename, dictionary *pstDict) {
    if (pstDict == NULL) {
        log_err("Dictionary is NULL.\n");
        return -1;
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        log_err("Failed to open file");
        return -1;
    }

    FILE *temp_file = fopen(TEMP_USER_INI_FILE_PATH, "w");
    if (!temp_file) {
        log_err("Failed to create temporary file");
        fclose(file);
        return -1;
    }

    char line[MAX_LINE_LENGTH];

    while (fgets(line, MAX_LINE_LENGTH, file)) {
        int updated = 0;
        if(line[0] != '#' && line[0] != '[' && line[0] != '\n')
        {
            for (int i = 0; i < (pstDict->n - 1); i++) {
                if (pstDict->val[i] != NULL) {
                    char section[64], key[64];
                    sscanf(pstDict->key[i], "%[^:]:%s", section, key);
                    if (strstr(line, key) != NULL) {
                        fprintf(temp_file, "%s=%s\n", key, pstDict->val[i]);
                        //found[i] = 1;
                        updated = 1;
                        break;
                    }
                }
            }
        }

        if (!updated) {
            fputs(line, temp_file);
        }
    }
    
    fclose(file);
    fclose(temp_file);


    if (remove(filename) != 0) {
        log_err("Failed to remove original file");
        return -1;
    }
    
    if (rename(TEMP_USER_INI_FILE_PATH, filename) != 0) {
        log_err("Failed to rename temporary file");
        return -1;
    }
    
    return 0;
}

int system_value_sync_to_sdcard(void)
{
    int i;
    dictionary *pstDict = NULL;
    char full_key[64] = {0};

    pstDict = iniparser_load(USER_INI_FILE_PATH);
    if (pstDict == NULL) {
        log_err("Failed to load %s\n", USER_INI_FILE_PATH);
        return -1;
    }

    get_air_init_param();
 
    for (i = 0; i < sizeof(wfb_params)/sizeof(wfb_params[0]); i++)
    {
        snprintf(full_key, sizeof(full_key), "%s:%s", wfb_params[i].section, wfb_params[i].key);
        //log_debug("full_key: %s", full_key);
        if (wfb_params[i].type == PARAM_TYPE_INT)
            iniparser_setint(pstDict, full_key, *(int *)wfb_params[i].air_param_ptr);
        else
            iniparser_setstring(pstDict, full_key, (char *)wfb_params[i].air_param_ptr);
    }

    for (i = 0; i < sizeof(telemetry_params)/sizeof(telemetry_params[0]); i++)
    {
        snprintf(full_key, sizeof(full_key), "%s:%s", telemetry_params[i].section, telemetry_params[i].key);
        //log_debug("full_key: %s", full_key);
        if (telemetry_params[i].type == PARAM_TYPE_INT)
            iniparser_setint(pstDict, full_key, *(int *)telemetry_params[i].air_param_ptr);
    }

    for (i = 0; i < sizeof(majestic_params)/sizeof(majestic_params[0]); i++)
    {
        snprintf(full_key, sizeof(full_key), "%s:%s", majestic_params[i].section, majestic_params[i].key);
        //log_debug("full_key: %s", full_key);
        if (majestic_params[i].type == PARAM_TYPE_INT)
            iniparser_setint(pstDict, full_key, *(int *)majestic_params[i].air_param_ptr);
        else
            iniparser_setstring(pstDict, full_key, (char *)majestic_params[i].air_param_ptr);
    }

    update_ini_file(USER_INI_FILE_PATH, pstDict);

    iniparser_freedict(pstDict);

    log_debug("System parameter synchronization to SD card completed.");

    return 0;
}

static int handle_config_param(ConfigParam *param, dictionary *ini_dict, bool *modified) {
    char full_key[64] = {0};

    snprintf(full_key, sizeof(full_key), "%s:%s", param->section, param->key);
    
    if (param->type == PARAM_TYPE_INT) {
        int newVal = iniparser_getint(ini_dict, full_key, 0);
        log_debug("full_key: %s = %d", full_key, newVal);
        int *currentVal = (int*)param->air_param_ptr;
        
        if (*currentVal != newVal) {
            if (param->min_val != param->max_val && (newVal < param->min_val || newVal > param->max_val))
            {
                log_err("Invalid value %d for %s", newVal, full_key);
                return -1;
            }

            char newValue[32];
            snprintf(newValue, sizeof(newValue), "%d", newVal);
            log_debug("Updating %s to %s", param->key, newValue);
            if(strcmp(param->section, "telemetry") == 0)
                modify_conf_value(TELEMETRY_FILE_PATH, param->key, newValue);
            else
                modify_conf_value(WFB_FILE_PATH, param->key, newValue);

            if (strcmp(param->key, "driver_txpower_override") == 0) {
                DynamicallyModifyTxPower(newVal);
            }
            else
                *modified = true;
            //return true;
        }
    }
    return 0;
}

static int handle_YAML_param(ConfigParam *param, dictionary *ini_dict, bool *modified)
{
    char full_key[64] = {0};

    snprintf(full_key, sizeof(full_key), "%s:%s", param->section, param->key);

    if (param->type == PARAM_TYPE_INT) {
        int newVal = iniparser_getint(ini_dict, full_key, 0);
        int *currentVal = (int*)param->air_param_ptr;

        log_debug("full_key: %s = %d  currentVal = %d", full_key, newVal, *currentVal);

        if (*currentVal != newVal) {
            if (param->min_val != param->max_val && 
                (newVal < param->min_val || newVal > param->max_val)) {
                log_err("Invalid value %d for %s", newVal, full_key);
                return -1;
            }

            char strVal[32];
            snprintf(strVal, sizeof(strVal), "%d", newVal);
            log_debug("Updating %s to %s", param->key, strVal);
            wifilink_yaml(YAML_CLI_SET, param->section, param->key, strVal);

            *currentVal = newVal;
            *modified = true;
        }
    }
    else
    {
        char *newVal = iniparser_getstring(ini_dict, full_key, NULL);
        log_debug("full_key: %s = %s", full_key, newVal);
        char *currentVal = (char*)param->air_param_ptr;

        if (strcmp(currentVal, newVal) != 0) {
            log_debug("Updating %s to %s", param->key, newVal);
            wifilink_yaml(YAML_CLI_SET, param->section, param->key, newVal);
            strcpy(currentVal, newVal);
            *modified = true;
        }
    }

    return 0;
}
