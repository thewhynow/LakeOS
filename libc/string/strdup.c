#include "../include/string.h"
#include "../../kernel/include/kmm.h"

char *strdup(const char *str){
    size_t len = strlen(str) + 1;
    char *ret = kmalloc(len);
    memcpy(ret, str, len);
    return ret;
}
