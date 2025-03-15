#ifndef _CONF_PAESER_H_
#define _CONF_PAESER_H_

#define MAX_LINE_LENGTH 256
#define MAX_KEY_LENGTH 128
#define MAX_VAL_LENGTH 128
#define MAX_KEYS 100

typedef struct {
    char **keys;
    char **values;
    size_t count;
} Config;

Config* confparser_load(const char *filename);
int confparser_getval(Config *config, const char *key, char *value, size_t value_len);
int confparser_setval(Config *config, const char *key, const char *value, const char *filename);
int confparser_getstring(Config *config, const char *key, char *value, size_t value_len);
int confparser_getint(Config *config, const char *key);
void  confparser_free(Config *config);
int save_config(const Config *config, const char *filename);
int modify_conf_value(const char *filename, const char *key, const char *newValue);

#endif //#define _CONF_PAESER_H_