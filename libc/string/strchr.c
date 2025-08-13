#include "../include/string.h"

char *strchr(char *str, int chr){
    while (*str)
        if (*str == chr)
            return str;
        else
            ++str;

    if (!chr) return str;

    return NULL;
}