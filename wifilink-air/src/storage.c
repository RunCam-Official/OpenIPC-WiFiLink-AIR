#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/statfs.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include "storage.h"
#include "debug.h"
#include "configuration.h"
#include "iniparser.h"
#include "wifilink.h"
#include "led.h"

#define PARTTITION_CHECK "/proc/partitions"
#define SD_ROOT "/mnt/mmcblk0p1"

#define UEVENT_BUFFER_SIZE 1024
#define SD_DEV_NAME_POS 5

pthread_t g_storageDetectThread = 0;
pthread_attr_t storage_thread_attr;
bool g_storageDetect_exit  = true;
bool storage_insert = false;
int g_hotplug_sock  = -1;
bool UpgradeFlag = false;

static char g_devName[] = "/dev/mmcblk0p1";

extern bool card_cfg_flag;

bool storage_insert_state(void)
{
    return storage_insert;
}

static int init_hotplug_sock(void)
{
    struct sockaddr_nl snl;
    const int          buffersize = 16 * 1024;
    struct timeval     timeout    = {1, 0};
    int                retval;
    memset(&snl, 0x00, sizeof(struct sockaddr_nl));
    snl.nl_family    = AF_NETLINK;
    snl.nl_pid       = getpid();
    snl.nl_groups    = 1;
    int hotplug_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (hotplug_sock == -1)
    {
        printf("error getting socket: %s", strerror(errno));
        return -1;
    }
    /* set receive buffersize */
    setsockopt(hotplug_sock, SOL_SOCKET, SO_RCVBUFFORCE, &buffersize, sizeof(buffersize));
    /* set receive timeout */
    setsockopt(hotplug_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
    retval = bind(hotplug_sock, (struct sockaddr *)&snl, sizeof(struct sockaddr_nl));
    if (retval < 0)
    {
        printf("bind failed: %s", strerror(errno));
        close(hotplug_sock);
        hotplug_sock = -1;
        return -1;
    }
    return hotplug_sock;
}


void WiFiLinkUpgrade(bool insert) {
    #if WIFILINK2
    const char *pKernelPath = "/mnt/mmcblk0p1/WiFiLink2-part0.bin";
    const char *pRootfsPath = "/mnt/mmcblk0p1/WiFiLink2-part1.bin";
    #else
    const char *pKernelPath = "/mnt/mmcblk0p1/WiFiLink-part0.bin";
    const char *pRootfsPath = "/mnt/mmcblk0p1/WiFiLink-part1.bin";
    #endif

    char cmdbuffer[256] = {"0"};

    if(insert)
    {
        if(access(pKernelPath, F_OK) == 0)
        {
            sprintf(cmdbuffer, "cp %s /tmp", pKernelPath);
            wifilink_system(cmdbuffer);
            if(access(UNDELETE_FILE_PATH, F_OK) != 0)
            {
                memset(cmdbuffer, 0, sizeof(cmdbuffer));
                sprintf(cmdbuffer, "rm %s", pKernelPath);
                wifilink_system(cmdbuffer);
            }
            UpgradeFlag = true;
        }

        if(access(pRootfsPath, F_OK) == 0)
        {
            memset(cmdbuffer, 0, sizeof(cmdbuffer));
            sprintf(cmdbuffer, "cp %s /tmp", pRootfsPath);
            wifilink_system(cmdbuffer);
            if(access(UNDELETE_FILE_PATH, F_OK) != 0)
            {
                memset(cmdbuffer, 0, sizeof(cmdbuffer));
                sprintf(cmdbuffer, "rm %s", pRootfsPath);
                wifilink_system(cmdbuffer);
            }
            UpgradeFlag = true;
        }

        if(UpgradeFlag)
        {
            if(access(UNDELETE_FILE_PATH, F_OK) == 0)
            {
                #if WIFILINK2
                wifilink_system("sysupgrade --kernel=/tmp/WiFiLink2-part0.bin --rootfs=/tmp/WiFiLink2-part1.bin -n -x");
                #else
                wifilink_system("sysupgrade --kernel=/tmp/WiFiLink-part0.bin --rootfs=/tmp/WiFiLink-part1.bin -n -x");
                #endif
            }
            else
            {
                #if WIFILINK2
                wifilink_system("sysupgrade --kernel=/tmp/WiFiLink2-part0.bin --rootfs=/tmp/WiFiLink2-part1.bin -n");
                #else
                wifilink_system("sysupgrade --kernel=/tmp/WiFiLink-part0.bin --rootfs=/tmp/WiFiLink-part1.bin -n");
                #endif
            }
            //wifilink_system("/tmp/upgrade_led.sh &");
            //set_led_state(&control, LED_BLINK_INFINITE, LED_GREEN, 500, 0);
        }
    }

}

void checkGskeyUseriniFile(void)
{
    uint64_t waitms = 500;
    
    uint64_t curTime = wifilink_get_time_ms();

    while (wifilink_get_time_ms() - curTime < waitms)
    {
        #if WIFILINK_FPV
        if(access(GS_KEY_FILE_PATH, F_OK) == 0 || access(USER_INI_FILE_PATH, F_OK) == 0)
            break;
        #else
        if(access(USER_INI_FILE_PATH, F_OK) == 0)
            break;
        #endif
    }
    #if WIFILINK_FPV
    if(access(GS_KEY_FILE_PATH, F_OK) != 0)
        wifilink_system("cp /etc/drone.key /mnt/mmcblk0p1/gs.key");
    if(access(USER_INI_FILE_PATH, F_OK) != 0)
        wifilink_system("cp /etc/user.ini /mnt/mmcblk0p1/user.ini");
    #endif

    #if WIFILINK_RUBYFPV
    if(access(RUBY_FPV_VERSION, F_OK) != 0)
        wifilink_system("cp /etc/ruby-version /mnt/mmcblk0p1/ruby-version");
    #endif
}

void *StorageDetectTask(void *argv)
{
    int ret = 0;

     g_hotplug_sock = init_hotplug_sock();

    if (g_hotplug_sock == -1)
    {
        pthread_exit(NULL);
    }

    ret = access(g_devName, F_OK);
    if(ret == 0)
        storage_insert = true;
    else
        storage_insert = false;

    if(storage_insert)
        checkGskeyUseriniFile();

    WiFiLinkUpgrade(storage_insert);

    while (g_storageDetect_exit == false)
    {
        char buf[UEVENT_BUFFER_SIZE] = {
            0,
        };
        char action[16] = {
            0,
        };

        usleep(50 * 1000);

        ret = recv(g_hotplug_sock, &buf, sizeof(buf), 0);
        
        if (ret == -1)
            continue;

        sscanf(buf, "%[^'@']", action);
        //log_debug("buf = %s", buf);
        if (!storage_insert_state() && strncmp(action, "add", sizeof(action)) == 0)
        {
            if (strstr(buf, &g_devName[SD_DEV_NAME_POS]))
            {
                log_info("sd card insert.");
                checkGskeyUseriniFile();
                card_cfg_flag = true;
                storage_insert = true;
            }
        }
        else if (storage_insert_state() && strncmp(action, "remove", sizeof(action)) == 0)
        {
            if (strstr(buf, &g_devName[SD_DEV_NAME_POS]))
            {
                log_info("sd card remove.");
                storage_insert = false;
                records_state_led();
            }
        }
    }

    close(g_hotplug_sock);

    pthread_exit(NULL);
}

void StorageStartDetectThread(void)
{
    int ret = 0;

    if (true != g_storageDetect_exit)
    {
        log_err("%s alread start\n", __func__);
        return;
    }

    g_storageDetect_exit = false;

    if(pthread_attr_init(&storage_thread_attr) != 0)
    {
        log_err("pthread_attr_init.");
        return;
    }

    if(pthread_attr_setstacksize(&storage_thread_attr, 1024*16))
    {
        log_err("pthread_attr_setstacksize.");
        return;
    }

    ret = pthread_create(&g_storageDetectThread, &storage_thread_attr, StorageDetectTask, NULL);
    if (0 != ret)
    {
        log_err("pthread create failed!");
    }

    return;
}

void StorageStopDetectThread(void)
{
    g_storageDetect_exit = true;
    pthread_join(g_storageDetectThread, NULL);
    pthread_attr_destroy(&storage_thread_attr);
}