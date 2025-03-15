#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <stdint.h>
#include "gpio.h"
#include "led.h"
#include "debug.h"
#include "iniparser.h"
#include "dictionary.h"
#include "conf_parser.h"
#include "storage.h"
#include "configuration.h"
#include "wifilink.h"

#define BUFFER_SIZE 64
#define TEMPERATURE_PATH "/sys/devices/virtual/mstar/msys/TEMP_R"

#define TEMPERATURE_WARNING 98
#define TEMPERATURE_POWEROFF 108

#define TEMP_R_DUMP 0

typedef enum {
    C_TEMP_STATE_NORMAL,
    C_TEMP_STATE_WARNING,
    C_TEMP_STATE_POWEROFF
} CHIP_TEMPERATURE_STATE;

typedef enum {
    ROUTER_MAVFWD=0,
    ROUTER_MAVLINK_ROUTERD,
    ROUTER_MSPOSD
} FLIGHT_CONTROL_ROUTER;

bool wifilink_main = true;
bool card_cfg_flag = true;

extern LEDControl control;
extern AirParam_s airParam;
extern bool UpgradeFlag;

void wifilink_handler_exit(void);

void wifilink_system(const char *command)
{
    system(command);
}

uint64_t wifilink_get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(1, &ts);

    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
}

void remove_newline(char *str) {
    if (str == NULL) return;

    size_t len = strlen(str);
    if (len > 0) {

        if (str[len - 1] == '\n') {
            str[len - 1] = '\0';
            len--;
        }

        if (len > 0 && str[len - 1] == '\r') {
            str[len - 1] = '\0';
        }
    }
}

int wifilink_yaml(int type, const char *param1, const char *param2, char *param3)
{
    char cmdstr[256] = {0};
    char gCmdBuf[256] = {0};

    //log_debug("%s %s %s", param1, param2, param3);
    switch (type)
    {
    case YAML_CLI_SET:
        sprintf(cmdstr, "yaml-cli -i %s -s .%s.%s %s", YAML_FILE_PATH, param1, param2, param3);
        break;
    case YAML_CLI_GET:
        sprintf(cmdstr, "yaml-cli -i %s -g .%s.%s", YAML_FILE_PATH, param1, param2);
        break;
    default:
        log_err("yaml cli command fail!\n");
        return -1;
        break;
    }
    //log_debug("cmd str: %s", cmdstr);
    FILE *fp = popen(cmdstr, "r");
    if (fp == NULL)
    {
        log_err("popen %s fail!", cmdstr);
        return -1;
    }

    if(fgets(gCmdBuf, sizeof(gCmdBuf), fp) != NULL)
    {
        //log_debug("gCmdBuf: %s", gCmdBuf);
        remove_newline(gCmdBuf);
        sprintf(param3, "%s", gCmdBuf);
    }
    pclose(fp);

    return 0;
}

int wifilink_yaml_getint(int type, const char *param1, const char *param2, int *gval)
{
    char valbuf[32] = {"0"};
    if(wifilink_yaml(type, param1, param2, valbuf) == -1)
        return -1;
    //log_debug("valbuf: %s", valbuf);
    *gval = strtol(valbuf, NULL, 0);
    return 0;
}

void msposd_chip_info_set(void)
{
    static int wait = 5;

    if(!wait)
    {
        if(airParam.telemetry_router == ROUTER_MSPOSD)
            wifilink_system("echo \"&L00 &F26 CPU:&C temp:&T\" > /tmp/MSPOSD.msg");
            //wifilink_system("echo \"&L00 &F26 CPU:&C &B temp:&T\" > /tmp/MSPOSD.msg");
            //wifilink_system("echo \"Chip info:&L00 &F32 CPU:&C temp:&T\" > /tmp/MSPOSD.msg");
        wait = 0xff;
    }
    if(wait != 0xff)
        wait--;
}

void temperature_handler(int ct)
{
    static int update_state_flag = 0;
    static int temperature = C_TEMP_STATE_NORMAL;

    //log_debug("ct = %d", ct);
    
    if(ct > TEMPERATURE_POWEROFF)
    {
        wifilink_system("poweroff");
        set_led_state(&control, LED_OFF, LED_ALL, 0, 0);
        log_warning("Automatic shutdown when the temperature exceeds 108 degrees Celsius!\n");
        while(1);
    }
    else if(ct > TEMPERATURE_WARNING)
    {
        if(C_TEMP_STATE_WARNING != temperature && 0 == update_state_flag)
        {
            temperature = C_TEMP_STATE_WARNING;
            update_state_flag = 1;
        }
    }
    else
    {
        if(C_TEMP_STATE_NORMAL != temperature && 0 == update_state_flag)
        {
            temperature = C_TEMP_STATE_NORMAL;
            update_state_flag = 1;
        }
    }

    if(update_state_flag)
    {
        //log_debug("temperature = %d", temperature);
        if(C_TEMP_STATE_NORMAL == temperature)
        {
            set_led_state(&control, LED_ON, LED_BLUE, 0, 0);
            sleep(1);
            set_led_state(&control, LED_OFF, LED_GREEN, 0, 0);
            
        }
        else if(C_TEMP_STATE_WARNING == temperature)
        {
            if(!control.running)
                set_led_state(&control, LED_BLINK_ALTERNATE, LED_UNDEFINED, 200, 0);
        }
        update_state_flag = 0;
    }
}

int temperature_value_dump(int temp_r)
{
    int fd;
    char buffer[64] = {0}, time_buffer[32] = {0};
    char filename[] = "/mnt/mmcblk0p1/temperature_dump.txt";
    struct tm *timeinfo;
    time_t rawtime;

    if (access("/mnt/mmcblk0p1", F_OK) == -1)
    {
        return EXIT_FAILURE;
    }

    if ((fd = open(filename, O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR)) == -1) {
        log_err("Failed to open file");
        return EXIT_FAILURE;
    }

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    sprintf(buffer, "%s temperature: %d\n", time_buffer, temp_r);

    if (write(fd, buffer, strlen(buffer)) == -1) {
        log_err("Failed to write to file");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);

    return EXIT_SUCCESS;
}

int read_chip_temperature(void)
{
    int fd;
    char buffer[BUFFER_SIZE];
    char *p;
    int temperature;

    if ((fd = open(TEMPERATURE_PATH, O_RDONLY)) == -1) {
        log_err("Error opening file");
        return EXIT_FAILURE;
    }

    if (read(fd, buffer, BUFFER_SIZE - 1) == -1) {
        log_err("Error reading from file");
        close(fd);
        return EXIT_FAILURE;
    }

    if (close(fd) == -1) {
        log_err("Error closing file");
        return EXIT_FAILURE;
    }

    buffer[BUFFER_SIZE - 1] = '\0';

    p = strtok(buffer, " \n");
    p = strtok(NULL, " \n");
    if (p != NULL) {

        temperature = atoi(p);
        temperature_handler(temperature);
        #if TEMP_R_DUMP
        temperature_value_dump(temperature);
        #endif
		return temperature;
    } else {
        log_err("Failed to parse temperature from the file.");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

}

int check_wifi_loading_state(int wait_t)
{
    struct timeval time;
    long int wait_time = 0;

    int fd = -1;
    char buffer[8] = {0};
    char *filename = "/sys/class/net/wlan0/operstate";
    char *keyword = "up";
    int found = 0;

    gettimeofday(&time, NULL);
    wait_time = time.tv_sec + wait_t;

    while(time.tv_sec < wait_time)
    {
        if (access(filename, F_OK) == 0)
        {
            if ((fd = open(filename, O_RDONLY)) == -1) {
                log_err("Error opening file");
                //return EXIT_FAILURE;
            }
            else
            {
                if (read(fd, buffer, sizeof(buffer)) > 0)
                {
                    if (NULL != strstr(buffer, keyword))
                        found = 1;
                }

                close(fd);
            }

            if(found)break;
        }

        gettimeofday(&time, NULL);
    }

    if(found) 
        set_led_state(&control, LED_ON, LED_BLUE, 0, 0);
    else
        set_led_state(&control, LED_BLINK_INFINITE, LED_BLUE, 100, 0);

    return found;
}

void handle_signal(int sig) {
    wifilink_handler_exit();
}

void wifilink_init(char *argv[])
{
    struct sigaction action;
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGHUP, &action, NULL);  //-1
    sigaction(SIGINT, &action, NULL);  //-2
    sigaction(SIGQUIT, &action, NULL); //-3
    sigaction(SIGTERM, &action, NULL); //-15
    
    wifilink_init_led();
    check_wifi_loading_state(atoi(argv[1]));
    StorageStartDetectThread();
    #if WIFILINK_FPV
    get_air_init_param();
    audio_state_led();
    #endif
}

void wifilink_handler_exit(void)
{
    wifilink_main = false;
}

int main(int argc, char *argv[]) {

    if(argc < 2)
    {
        log_err("No parameters added!");
        return EXIT_FAILURE;
    }

    wifilink_init(argv);

	while(wifilink_main)
	{
        msposd_chip_info_set();
		read_chip_temperature();
        #if WIFILINK_FPV
        if(card_cfg_flag)
        {
            if (access(USER_INI_FILE_PATH, F_OK) == 0)
                CardConfigurationCheck();
            card_cfg_flag = false;
        }
        #endif
        if(UpgradeFlag)
        {
            UpgradeFlag = false;
            set_led_state(&control, LED_BLINK_INFINITE, LED_GREEN, 300, 0);
        }

        #if WIFILINK_FPV && RC_SET_PARAM_SYNC
        if(access("/tmp/msp_menu_save_value_to_system", F_OK) == 0)
        {
            system_value_sync_to_sdcard();
            audio_state_led();
            remove("/tmp/msp_menu_save_value_to_system");
        }
        #endif

        if(access(SYSUPGRADE_SKIP_REBOOT, F_OK) == 0)
            wifilink_handler_exit();

		sleep(1);
	}

    wifilink_led_stop_thread();
    StorageStopDetectThread();
    sleep(1);
    if(access(SYSUPGRADE_SKIP_REBOOT, F_OK) == 0)
    {
        log_debug();
        wifilink_gpio_set_val(BLUE_LED, GPIO_LOW);
        wifilink_gpio_set_val(GREEN_LED, GPIO_LOW);
        wifilink_system("poweroff");
    }
    log_info("wifilink exit.");
	return EXIT_SUCCESS;
}
