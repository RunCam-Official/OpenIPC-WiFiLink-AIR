#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "conf_parser.h"
#include "debug.h"

#define LINE_MAX 1024

Config* confparser_load(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        log_err("Error opening %s file", filename);
        return NULL;
    }

    Config *config = (Config *)malloc(sizeof(Config));
    config->keys = NULL;
    config->values = NULL;
    config->count = 0;

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        char key[MAX_KEY_LENGTH], value[MAX_VAL_LENGTH];
        if (sscanf(line, "%127[^=]=%127s", key, value) == 2) {
            config->count++;
            config->keys = realloc(config->keys, config->count * sizeof(char *));
            config->values = realloc(config->values, config->count * sizeof(char *));
            config->keys[config->count - 1] = strdup(key);
            config->values[config->count - 1] = strdup(value);
        }
    }

    fclose(file);
    return config;
}

int confparser_getval(Config *config, const char *key, char *value, size_t value_len) {
    for (size_t i = 0; i < config->count; i++) {
        if (strcmp(config->keys[i], key) == 0) {
            strncpy(value, config->values[i], value_len - 1);
            value[value_len - 1] = '\0';
            return 0;
        }
    }
    return -1;
}

int confparser_getint(Config *config, const char *key) {
    char valbuf[128] = {"0"};
    if(confparser_getval(config, key, valbuf, sizeof(valbuf)) == -1)
        return -1;

    return (int)strtol(valbuf, NULL, 0);
}

int confparser_getstring(Config *config, const char *key, char *value, size_t value_len) {
    if(confparser_getval(config, key, value, value_len) == -1)
        return -1;

    return 0;
}

int confparser_setval(Config *config, const char *key, const char *value, const char *filename) {
    for (size_t i = 0; i < config->count; i++) {
        if (strcmp(config->keys[i], key) == 0) {
            free(config->values[i]);
            config->values[i] = strdup(value);
            return save_config(config, filename);
        }
    }

    return -1;
}

int save_config(const Config *config, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        log_err("Error opening %s file", filename);
        return -1;
    }

    for (size_t i = 0; i < config->count; i++) {
        fprintf(file, "%s=%s\n", config->keys[i], config->values[i]);
    }

    fclose(file);
    return 0;
}

void confparser_free(Config *config) {
    if (config) {
        for (size_t i = 0; i < config->count; i++) {
            free(config->keys[i]);
            free(config->values[i]);
        }
        free(config->keys);
        free(config->values);
        free(config);
    }
}

int move_file(const char *source, const char *destination) {
    FILE *src = fopen(source, "rb");
    if (!src) {
        log_err("Error opening source file %s", source);
        return -1;
    }

    FILE *dst = fopen(destination, "wb");
    if (!dst) {
        log_err("Error opening destination file %s", destination);
        fclose(src);
        return -1;
    }

    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, bytes, dst) != bytes) {
            log_err("Error writing to destination file");
            fclose(src);
            fclose(dst);
            return -1;
        }
    }

    fclose(src);
    fclose(dst);

    if (remove(source) != 0) {
        perror("Error deleting source file");
        return -1;
    }

    return 0;
}

int modify_conf_value(const char *filename, const char *key, const char *newValue) {
    if (!filename || !key || !newValue) {
        fprintf(stderr, "Invalid arguments\n");
        return -1;
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return -1;
    }

    FILE *tempFile = fopen("temp.conf", "w");
    if (!tempFile) {
        perror("Failed to create temporary file");
        fclose(file);
        return -1;
    }

    char line[LINE_MAX];
    int modified = 0;

    while (fgets(line, sizeof(line), file)) {
        char currentKey[LINE_MAX];
        char currentValue[LINE_MAX];

        if (line[0] == '#' || line[0] == '\n') {
            fputs(line, tempFile);
            continue;
        }

        if (sscanf(line, "%[^=]=%s", currentKey, currentValue) == 2) {
            if (strcmp(currentKey, key) == 0) {
                fprintf(tempFile, "%s=%s\n", key, newValue);
                modified = 1;
            } else {
                fputs(line, tempFile);
            }
        } else {
            fputs(line, tempFile);
        }
    }

    fclose(file);
    fclose(tempFile);

    if (!modified) {
        remove("temp.conf");
        return -1;
    }

    if(move_file("temp.conf", filename) != 0)
    {
        log_err("Failed to replace original file");
        return -1;
    }

    return 0;
}
