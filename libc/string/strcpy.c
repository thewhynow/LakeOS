#include "../include/string.h"

char *strcpy(char *dest, const char *src){
    memcpy(dest, src, strlen(src));
    return dest;
}
