#include <string.h>
#include <ctype.h>
#include "strlib.h"

#define ASCIILINESZ 1023

void strlwc(const char* s, char* result, unsigned int r_size)
{
    int i = 0;

    if (s == NULL)
        return;

    if (NULL == result)
        return;

    if (0 == r_size)
        return;

    memset(result, 0, r_size);

    while (s[i] && (i < (int)(r_size - 1)))
    {
        result[i] = (char)tolower((int)s[i]);
        i++;
    }
    result[r_size - 1] = (char)0;
}

void strupc(const char* s, char* result, unsigned int r_size)
{
    int i = 0;

    if (s == NULL)
        return;

    if (NULL == result)
        return;

    if (0 == r_size)
        return;

    memset(result, 0, r_size);

    while (s[i] && (i < (int)(r_size - 1)))
    {
        result[i] = (char)toupper((int)s[i]);
        i++;
    }
    result[r_size - 1] = (char)0;
}

char* strskp(char* s)
{
    char* skip = s;
    if (s == NULL)
        return NULL;
    while (isspace((int)*skip) && *skip)
        skip++;
    return skip;
}

void strcrop(const char* s, char* result, unsigned int r_size)
{
    char* last = NULL;

    if (s == NULL)
        return;

    if (NULL == result)
        return;

    if (0 == r_size)
        return;

    memset(result, 0, r_size);
    strncpy(result, s, r_size - 1);
    result[r_size - 1] = '\0';

    last = result + strlen(result);
    while (last > result)
    {
        if (!isspace((int)*(last - 1)))
            break;
        last--;
    }
    *last = (char)0;
}

void strstrip(const char* s, char* result, unsigned int r_size)
{
    char* last = NULL;

    if (s == NULL)
        return;

    if (NULL == result)
        return;

    if (0 == r_size)
        return;

    while (isspace((int)*s) && *s)
        s++;

    memset(result, 0, r_size);
    strncpy(result, s, r_size - 1);
    result[r_size - 1] = '\0';

    last = result + strlen(result);
    while (last > result)
    {
        if (!isspace((int)*(last - 1)))
            break;
        last--;
    }
    *last = (char)0;
}

void strstrip_for_getstring(const char* s, char* result, unsigned int r_size)
{
    if (s == NULL)
        return;

    if (NULL == result)
        return;

    if (0 == r_size)
        return;

    char* last = NULL;

    while (isspace((int)*s) && *s)
        s++;

    memset(result, 0, r_size);
    strncpy(result, s, r_size - 1);
    result[r_size - 1] = '\0';

    last = result + strlen(result);
    while (last > result)
    {
        if (!isspace((int)*(last - 1)))
            break;
        last--;
    }
    *last = (char)0;

    // check ':' nearby blank & take them off
    char* pCheck = result;
    char* pBlankFound;
    char* substr = NULL;
    char* l_2    = NULL;
    char* l_temp = NULL;

    l_2 = malloc(ASCIILINESZ + 1);
    if (NULL == l_2)
        goto EXIT;

    l_temp = malloc(ASCIILINESZ + 1);
    if (NULL == l_temp)
        goto EXIT;

    memset(l_2, 0, ASCIILINESZ + 1);
    memset(l_temp, 0, ASCIILINESZ + 1);

    // check if have blank in string
    while (!isspace((int)*pCheck) && *pCheck)
        pCheck++;
    if (pCheck == last)
        goto EXIT;

    // find ':'
    pCheck = result;
    do
    {
        if (*pCheck == ':')
            break; // find first ':', we don't handle secondary ':'
        pCheck++;
    } while (pCheck != last);

    if (pCheck == last)
        goto EXIT; // can't find ':'

    // blank before ':'
    if (result != pCheck)
    {
        pBlankFound = pCheck - 1;
        while (isspace((int)*pBlankFound))
        {
            *pBlankFound = 0;
            pBlankFound--;
        }
        strcpy(l_2, result);
    }

    substr = l_2;
    snprintf(l_temp, ASCIILINESZ + 1, "%s:", substr);

    // blank after ':'
    pBlankFound = pCheck + 1;
    while (isspace((int)*pBlankFound) && *pBlankFound)
    {
        pBlankFound++;
    }
    memset(l_2, 0, ASCIILINESZ + 1);
    snprintf(l_2, ASCIILINESZ + 1, "%s%s", l_temp, pBlankFound);

    strncpy(result, l_2, r_size);
EXIT:
    if (l_2)
        free(l_2);
    if (l_temp)
        free(l_temp);
}