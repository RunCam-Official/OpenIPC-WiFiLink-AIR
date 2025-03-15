#ifndef _STRLIB_H_
#define _STRLIB_H_

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void strlwc(const char* s, char* result, unsigned int r_size);
void strupc(const char* s, char* result, unsigned int r_size);
void strcrop(const char* s, char* result, unsigned int r_size);
void strstrip(const char* s, char* result, unsigned int r_size);
void strstrip_for_getstring(const char* s, char* result, unsigned int r_size);
char* strskp(char* s);

#ifdef __cplusplus
}
#endif

#endif
