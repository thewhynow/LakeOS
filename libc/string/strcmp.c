#include "../include/string.h"

int strcmp(const char *a, const char *b){
    size_t alen = strlen(a),
           blen = strlen(b);
    return memcmp(a, b, alen < blen ? alen : blen);
}
