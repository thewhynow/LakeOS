// #include <limits.h>
// #include <stdbool.h>
#include <stdarg.h>
#include "../include/stdio.h"
#include "../include/string.h"
#include "../include/stdlib.h"
#include "../include/types.h"

static bool print(const char* data, size_t length){
    for (size_t i = 0; i < length; ++i)
        if (putchar(data[i]) == EOF)
            return false;
    return true;
}

static bool printint(int n){
    char buff[10];
    
    int i = 0;

    if (n < 0){
        n = -n;
        if (putchar('-') == EOF)
            return false;
    }

    do {
        buff[i++] = '0' + (n % 10);
        n /= 10;
    } while(n);

    while (i--)
        if(putchar(buff[i]) == EOF)
            return false;

    return true;
}

static bool printlong(long n){
    char buff[30];
    
    int i = 0;

    do {
        buff[i++] = '0' + ((unsigned long)n % 10);
        n /= 10;
    } while(n);

    while (i--)
        if(putchar(buff[i]) == EOF)
            return false;

    return true;
}

static void printptr(void *ptr){
 uint32_t p = (uint32_t)(size_t)ptr;
    const char hex[] = "0123456789ABCDEF";

    putchar('0');
    putchar('x');
    for (int i = 7; i >= 0; i--) {
        putchar(hex[(p >> (i * 4)) & 0xF]);
    }
}


int printf(const char* restrict format, ...){
    va_list params;
    va_start(params, format);

    int bytes_written = 0;

    while (*format){
        if (*format == '%'){
            switch(*++format){
                case 'c': {
                    putchar(va_arg(params, int));

                    ++bytes_written;

                    break;
                }
                
                case 's': {
                    const char* str = va_arg(params, const char*);
                    size_t len = strlen(str);

                    print(str, len);

                    bytes_written += len;

                    break;
                }

                case 'd':
                case 'i': {
                    int i = va_arg(params, int);
                    
                    printint(i);

                    do {
                        ++bytes_written;
                        i /= 10;
                    } while(i);

                    break;
                }

                case 'p': {
                    void *p = va_arg(params, void*);
                    printptr(p);
                    break;
                }

                default: {
                    putchar(*format);
                    ++bytes_written;

                    break;
                }
            }
            ++format;
        }
        else
            putchar(*format++);
    }

    va_end(params);

    return bytes_written;
}
