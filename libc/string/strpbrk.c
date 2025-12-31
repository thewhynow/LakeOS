#include "../include/string.h"

char *strpbrk(char *str, char *set){
    for (; *set; ++set){
        char *match = strchr(str, *set);

        if (match) return match;
    }

    return NULL;
}
