#include <string.h>

#ifdef __is_libk

#include <kernel/kmm.h>

char *strdup(const char *str){
    size_t len = strlen(str) + 1;
    char *ret = kmalloc(len);
    memcpy(ret, str, len);
    return ret;
}

#else

/* blah blah blah */

#endif
