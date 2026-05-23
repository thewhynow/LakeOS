#include "../include/string.h"

char *strchr(const char *str, int chr){
    while (*str)
        if (*str == chr)
            return (char*) str;
        else
            ++str;

    if (!chr) return (char*) str;

    return NULL;
}
