#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <unistd.h>
#include <stdbool.h>

bool storage_insert_state(void);
void StorageStartDetectThread(void);
void StorageStopDetectThread(void);

#endif //#define _STORAGE_H_