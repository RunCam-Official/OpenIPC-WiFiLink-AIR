#ifndef _WIFILINK_H_
#define _WIFILINK_H_

#include <stdint.h>

#define WIFILINK_FPV 1
#define WIFILINK_RUBYFPV 0

#define WIFILINK2   0

#define YAML_FILE_PATH  "/etc/majestic.yaml"
#define WFB_FILE_PATH  "/etc/wfb.conf"
#define TELEMETRY_FILE_PATH  "/etc/telemetry.conf"
#define USER_INI_FILE_PATH  "/mnt/mmcblk0p1/user.ini"
//#define USER_INI_FILE_PATH  "/tmp/user.ini"
#define TEMP_USER_INI_FILE_PATH  "/mnt/mmcblk0p1/temp.ini"
//#define TEMP_USER_INI_FILE_PATH  "/tmp/temp.ini"
#define GS_KEY_FILE_PATH  "/mnt/mmcblk0p1/gs.key"
#define UNDELETE_FILE_PATH  "/mnt/mmcblk0p1/undelete.txt"
#define RUBY_FPV_VERSION    "/mnt/mmcblk0p1/ruby-version"
#define SYSUPGRADE_SKIP_REBOOT "/tmp/skip_reboot"

typedef int (*CardConfigCallback)(void);

typedef enum
{
    YAML_CLI_SET,
    YAML_CLI_GET,
} YamlToolParamType;

void wifilink_system(const char *command);
int wifilink_yaml(int type, const char *param1, const char *param2, char *param3);
//int wifilink_yaml_getint(int type, const char *param1, const char *param2);
int wifilink_yaml_getint(int type, const char *param1, const char *param2, int *gval);
uint64_t wifilink_get_time_ms(void);

#endif //#define _WIFILINK_H_