#include "../include/string.h"

char *strtok(char *str, char *sep){
    static char *last;

    if (str) last = str;
    if (!last) return NULL;

    while (*last && strchr(sep, *last)) ++last;

    if (!*last){
        last = NULL;
        return NULL;
    }

    char *start = last;
    char *match = strpbrk(last, sep);

    if (match){
        *match = '\0';
        last = match + 1;
    }
    else
        last = NULL;

    return start;
}
