#include "../include/string.h"

char *strpbrk(const char *str, const char *set){
    for (; *set; ++set){
        char *match = strchr(str, *set);

        if (match) return match;
    }

    return NULL;
}
