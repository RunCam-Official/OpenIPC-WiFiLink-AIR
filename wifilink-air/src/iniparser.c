#include <assert.h>
#include "strlib.h"
#include "iniparser.h"
#include "debug.h"

#define ASCIILINESZ (8191)
#define INI_INVALID_KEY ((char *)-1)

typedef enum { LINE_UNPROCESSED, LINE_ERROR, LINE_EMPTY, LINE_COMMENT, LINE_SECTION, LINE_VALUE } line_status;

int iniparser_getnsec(dictionary *d)
{
    int i;
    int nsec;

    if (d == NULL)
        return -1;
    nsec = 0;
    for (i = 0; i < d->size; i++)
    {
        if (d->key[i] == NULL)
            continue;
        if (strchr(d->key[i], ':') == NULL)
        {
            nsec++;
        }
    }
    return nsec;
}

char *iniparser_getsecname(dictionary *d, int n)
{
    int i;
    int foundsec;

    if ((d == NULL) || (n < 0))
        return NULL;
    foundsec = 0;
    for (i = 0; i < d->size; i++)
    {
        if (d->key[i] == NULL)
            continue;
        if (strchr(d->key[i], ':') == NULL)
        {
            foundsec++;
            if (foundsec > n)
                break;
        }
    }
    if (foundsec <= n)
    {
        return NULL;
    }
    return d->key[i];
}

char *iniparser_getstring(dictionary *d, const char *key, char *def)
{
    char  lc_key[ASCIILINESZ + 1]        = {0};
    char *sval                           = NULL;
    char  non_blank_key[ASCIILINESZ + 1] = {0};

    if ((d == NULL) || (key == NULL))
        return def;

    // Remove blanks at the beginning and the end of a string.
    strstrip_for_getstring(key, non_blank_key, ASCIILINESZ + 1);

    strlwc(non_blank_key, lc_key, ASCIILINESZ + 1);
    sval = dictionary_get(d, lc_key, def);
    return sval;
}

int iniparser_getint(dictionary *d, const char *key, int notfound)
{
    char *str = NULL;

    str = iniparser_getstring(d, key, INI_INVALID_KEY);

    if (str == INI_INVALID_KEY)
        return notfound;

    return (int)strtol(str, NULL, 0);
}

unsigned int iniparser_getunsignedint(dictionary *d, const char *key, unsigned int notfound)
{
    char *str;

    str = iniparser_getstring(d, key, INI_INVALID_KEY);
    if (str == INI_INVALID_KEY)
        return notfound;
    return (unsigned int)strtoul(str, NULL, 0);
}

double iniparser_getdouble(dictionary *d, char *key, double notfound)
{
    char *str;

    str = iniparser_getstring(d, key, INI_INVALID_KEY);
    if (str == INI_INVALID_KEY)
        return notfound;
    return atof(str);
}

int iniparser_getboolean(dictionary *d, const char *key, int notfound)
{
    char *c;
    int   ret;

    c = iniparser_getstring(d, key, INI_INVALID_KEY);
    if (c == INI_INVALID_KEY)
        return notfound;
    if ((c[0] == 'y') || (c[0] == 'Y') || (c[0] == '1') || (c[0] == 't') || (c[0] == 'T'))
    {
        ret = 1;
    }
    else if ((c[0] == 'n') || (c[0] == 'N') || (c[0] == '0') || (c[0] == 'f') || (c[0] == 'F'))
    {
        ret = 0;
    }
    else
    {
        ret = notfound;
    }
    return ret;
}

int iniparser_find_entry(dictionary *ini, char *entry)
{
    int found = 0;
    if (iniparser_getstring(ini, entry, INI_INVALID_KEY) != INI_INVALID_KEY)
    {
        found = 1;
    }
    return found;
}

int iniparser_setstring(dictionary *ini, const char *entry, char *val)
{
    char lwc_key[ASCIILINESZ + 1] = {0};
    strlwc(entry, lwc_key, ASCIILINESZ + 1);
    return dictionary_set(ini, lwc_key, val);
}

int iniparser_setint(dictionary *ini, const char *entry, int val)
{
    char str_val[ASCIILINESZ + 1] = {0};
    snprintf(str_val, ASCIILINESZ + 1, "%d", val);
    return iniparser_setstring(ini, entry, str_val);
}

void iniparser_unset(dictionary *ini, char *entry)
{
    char lwc_key[ASCIILINESZ + 1] = {0};
    strlwc(entry, lwc_key, ASCIILINESZ + 1);
    dictionary_unset(ini, lwc_key);
}

static line_status iniparser_line(char *input_line, char *section, char *key, char *value)
{
    line_status sta;
    char *      line = NULL;
    int         len;

    line = malloc(ASCIILINESZ + 1);
    if (!line)
        return LINE_UNPROCESSED;

    if (input_line != NULL)
    {
        char *strip_line = NULL;
        strip_line       = malloc(ASCIILINESZ + 1);
        strstrip(input_line, strip_line, ASCIILINESZ + 1);
        strncpy(line, strip_line, ASCIILINESZ);
        free(strip_line);
    }
    line[ASCIILINESZ] = '\0';

    len = (int)strlen(line);

    sta = LINE_UNPROCESSED;
    if (len < 1)
    {
        /* Empty line */
        sta = LINE_EMPTY;
    }
    else if (line[0] == '#')
    {
        /* Comment line */
        sta = LINE_COMMENT;
    }
    else if ((line[0] == '[') && (line[len - 1] == ']'))
    {
        /* Section name */
        sscanf(line, "[%[^]]", section);
        if (section != NULL)
        {
            char *strip_section = NULL;
            char *lwc_section   = NULL;
            strip_section       = malloc(ASCIILINESZ + 1);
            lwc_section         = malloc(ASCIILINESZ + 1);
            memset(strip_section, 0, ASCIILINESZ + 1);
            memset(lwc_section, 0, ASCIILINESZ + 1);
            strstrip(section, strip_section, ASCIILINESZ + 1);
            strlwc(strip_section, lwc_section, ASCIILINESZ + 1);
            strncpy(section, lwc_section, ASCIILINESZ);
            free(strip_section);
            free(lwc_section);
        }
        sta = LINE_SECTION;
    }
    else if ((sscanf(line, "%[^=] = \"%[^\"]\"", key, value) == 2)
             || (sscanf(line, "%[^=] = '%[^\']'", key, value) == 2)
             || (sscanf(line, "%[^=] = %[^;#]", key, value) == 2))
    {
        /* Usual key=value, with or without comments */
        if (key != NULL)
        {
            char *strip_key = NULL;
            char *lwc_key   = NULL;
            strip_key       = malloc(ASCIILINESZ + 1);
            lwc_key         = malloc(ASCIILINESZ + 1);
            memset(strip_key, 0, ASCIILINESZ + 1);
            memset(lwc_key, 0, ASCIILINESZ + 1);
            strstrip(key, strip_key, ASCIILINESZ + 1);
            strlwc(strip_key, lwc_key, ASCIILINESZ + 1);
            strncpy(key, lwc_key, ASCIILINESZ);
            free(strip_key);
            free(lwc_key);
        }
        if (value != NULL)
        {
            char *strip_value = NULL;
            strip_value       = malloc(ASCIILINESZ + 1);
            memset(strip_value, 0, ASCIILINESZ + 1);
            strstrip(value, strip_value, ASCIILINESZ + 1);
            strncpy(value, strip_value, ASCIILINESZ);
            free(strip_value);
        }
        /*
         * sscanf cannot handle '' or "" as empty values
         * this is done here
         */
        if (value != NULL && (!strcmp(value, "\"\"") || (!strcmp(value, "''"))))
        {
            value[0] = 0;
        }
        sta = LINE_VALUE;
    }
    else if ((sscanf(line, "%[^=] = %[;#]", key, value) == 2) || (sscanf(line, "%[^=] %[=]", key, value) == 2))
    {
        /*
         * Special cases:
         * key=
         * key=;
         * key=#
         */
        if (key != NULL)
        {
            char *strip_key = NULL;
            char *lwc_key   = NULL;
            strip_key       = malloc(ASCIILINESZ + 1);
            lwc_key         = malloc(ASCIILINESZ + 1);
            memset(strip_key, 0, ASCIILINESZ + 1);
            memset(lwc_key, 0, ASCIILINESZ + 1);
            strstrip(key, strip_key, ASCIILINESZ + 1);
            strlwc(strip_key, lwc_key, ASCIILINESZ + 1);
            strncpy(key, lwc_key, ASCIILINESZ);
            free(strip_key);
            free(lwc_key);
        }
        value[0] = 0;
        sta      = LINE_VALUE;
    }
    else
    {
        /* Generate syntax error */
        sta = LINE_ERROR;
    }
    free(line);
    return sta;
}

dictionary *iniparser_load(const char *ininame)
{
    FILE *in;

    char *line    = NULL;
    char *section = NULL;
    char *key     = NULL;
    char *tmp     = NULL;
    char *val     = NULL;
    char *substr1 = NULL;
    char *substr2 = NULL;

    int last = 0;
    int len;
    int lineno = 0;
    int errs   = 0;

    dictionary *dict;

    if (ininame == NULL)
    {
        return NULL;
    }

    in = fopen(ininame, "r");
    if (in == NULL)
    {
        fprintf(stderr, "iniparser: cannot open %s\n", ininame);
        return NULL;
    }
    dict = dictionary_new(0);
    if (!dict)
    {
        fclose(in);
        return NULL;
    }

    line = malloc(ASCIILINESZ + 1);
    if (!line)
        goto EXIT;
    section = malloc(ASCIILINESZ + 1);
    if (!section)
        goto EXIT;
    key = malloc(ASCIILINESZ + 1);
    if (!key)
        goto EXIT;
    tmp = malloc(ASCIILINESZ + 1);
    if (!tmp)
        goto EXIT;
    val = malloc(ASCIILINESZ + 1);
    if (!val)
        goto EXIT;

    memset(line, 0, ASCIILINESZ + 1);
    memset(section, 0, ASCIILINESZ + 1);
    memset(key, 0, ASCIILINESZ + 1);
    memset(tmp, 0, ASCIILINESZ + 1);
    memset(val, 0, ASCIILINESZ + 1);
    last = 0;

    while (fgets(line + last, ASCIILINESZ + 1 - last, in) != NULL)
    {
        int end = 0;
        lineno++;
        len = (int)strlen(line);
        end = len - 1;
        /* Safety check against buffer overflows */
        if ((len == ASCIILINESZ) && (line[end] != '\n'))
        {
            fprintf(stderr, "iniparser: input line too long in %s (%d)\n", ininame, lineno);
            goto EXIT;
        }
        /* Get rid of \n and spaces at end of line */
        while ((len > 0) && ((line[end] == '\n') || (line[end] == '\r') || (isspace(line[end]))))
        {
            line[end] = 0;
            len--;
            end = len - 1;
        }
        /* Detect multi-line */
        if ((len > 0) && (line[end] == '\\'))
        {
            /* Multi-line value */
            last = end;
            continue;
        }
        else
        {
            last = 0;
        }
        switch (iniparser_line(line, section, key, val))
        {
        case LINE_EMPTY:
        case LINE_COMMENT:
            break;

        case LINE_SECTION:
            errs = dictionary_set(dict, section, NULL);
            break;

        case LINE_VALUE:
            // sprintf(tmp, "%s:%s", section, key);
            substr1 = section;
            substr2 = key;
            snprintf(tmp, ASCIILINESZ + 1, "%s:%s", substr1, substr2);
            errs = dictionary_set(dict, tmp, val);
            break;

        case LINE_ERROR:
            fprintf(stderr, "iniparser: syntax error in %s (%d):\n", ininame, lineno);
            fprintf(stderr, "-> %s\n", line);
            errs++;
            break;

        default:
            break;
        }
        memset(line, 0, ASCIILINESZ);
        last = 0;
        if (errs < 0)
        {
            fprintf(stderr, "iniparser: memory allocation failure\n");
            break;
        }
    }
    if (errs)
    {
    EXIT:
        dictionary_del(dict);
        dict = NULL;
    }
    fclose(in);
    if (line)
        free(line);
    if (section)
        free(section);
    if (key)
        free(key);
    if (tmp)
        free(tmp);
    if (val)
        free(val);
    return dict;
}

void iniparser_freedict(dictionary *d)
{
    dictionary_del(d);
}

char *iniparser_getVal(dictionary *d, char *section, int i)
{
    if ((d == NULL))
        return NULL;
    char lwc_section[ASCIILINESZ + 1] = {0};
    strlwc(section, lwc_section, ASCIILINESZ + 1);
    return dictionary_getVal(d, lwc_section, i);
}

int iniparser_getNumberOfSection(dictionary *d, char *section)
{
    char lwc_section[ASCIILINESZ + 1] = {0};
    strlwc(section, lwc_section, ASCIILINESZ + 1);
    return dictionary_getNumberOfSection(d, lwc_section);
}

const char **ParserStrToArray(const char *inputStr, int *count)
{
    char *       str       = NULL;
    const char **strArray  = NULL;
    int          leftIndex = -1, rightIndex = -2;
    int          i   = 0;
    int          len = 0;

    for (i = 0; i < (int)strlen(inputStr); i++)
    {
        if (*(inputStr + i) == '{')
        {
            leftIndex = i + 1;
            break;
        }
    }
    for (i = strlen(inputStr) - 1; i >= 0; i--)
    {
        if (*(inputStr + i) == '}')
        {
            rightIndex = i - 1;
            break;
        }
    }

    if (leftIndex > rightIndex)
    {
        return NULL;
    }
    len = rightIndex - leftIndex + 1;

    str = (char *)malloc(len + 1);
    if (str)
    {
        strncpy(str, inputStr + leftIndex, len);
        str[len]  = '\0';
        char *pch = strtok(str, " ,");
        while (pch != NULL)
        {
            pch = strtok(NULL, " ,");
            *count += 1;
        }
        free(str);

        if (*count)
        {
            const char **token = NULL;
            token              = (const char **)malloc(sizeof(char *) * (*count + 1) + len + 1);
            if (token)
            {
                memset(token, 0, sizeof(char *) * (*count + 1) + len + 1);
                str = (char *)(token + (*count + 1));
                strncpy(str, inputStr + leftIndex, len);
                str[len]  = '\0';
                i         = 0;
                char *pch = strtok(str, ",");
                while (pch != NULL)
                {
                    token[i++] = pch;
                    pch        = strtok(NULL, ",");
                }
                strArray = token;
            }
        }
    }

    return strArray;
}
