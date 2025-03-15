#ifndef _DEBUG_H_
#define _DEBUG_H_

#define LOG_WIFILINK_NONE    0
#define LOG_WIFILINK_ERR     1
#define LOG_WIFILINK_WARNING 2
#define LOG_WIFILINK_INFO    3
#define LOG_WIFILINK_DEBUG   4
#define LOG_WIFILINK         LOG_WIFILINK_DEBUG

#if LOG_WIFILINK >= LOG_WIFILINK_DEBUG
#define log_dump(fmt, ...) printf("\33[1;36m" fmt "\33[0m\n", ##__VA_ARGS__)
#else
#define log_dump(fmt, ...)
#endif

#if LOG_WIFILINK >= LOG_WIFILINK_DEBUG
#define log_debug(fmt, ...)                                                                                                                          \
    printf("[D]:[%s][%d] " fmt "\33[0m\n",                                                                                                                    \
           __func__, __LINE__, ##__VA_ARGS__)
#else
#define log_debug(fmt, ...)
#endif

#if LOG_WIFILINK >= LOG_WIFILINK_INFO
#define log_info(fmt, ...)                                                                                                                           \
    printf("\33[1;32m"                                                                                                                               \
           "[I]:" fmt "\33[0m\n",                                                                                                                    \
           ##__VA_ARGS__)
#else
#define log_info(fmt, ...)
#endif

#if LOG_WIFILINK >= LOG_WIFILINK_WARNING
#define log_warning(fmt, ...)                                                                                                                        \
    printf("\33[1;33m"                                                                                                                               \
           "[W]:" fmt "\33[0m\n",                                                                                                                    \
           ##__VA_ARGS__)
#else
#define log_warning(fmt, ...)
#endif

#if LOG_WIFILINK >= LOG_WIFILINK_ERR
#define log_err(fmt, ...)                                                                                                                            \
    printf("\33[1;31m"                                                                                                                               \
           "[E]:" fmt "\33[0m\n",                                                                                                                    \
           ##__VA_ARGS__)
#else
#define log_err(fmt, ...)
#endif

#endif //#define _DEBUG_H_