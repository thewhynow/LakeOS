#ifndef _SRV_H
#define _SRV_H

/**
 * services are my way of doing multithreaded
 *  background bullshit ;)
 * essentially this registers a function to be called
 *  at the entry point of every syscall.
 */

/**
 * NO idea where this is coming from
 */
#include <stdbool.h>

typedef struct {
    void (*service_f)(void *args);
    void *args;
    bool enabled;
} service_t;

int  register_service(void (*service_f)(void *args), void *args);

service_t *get_service(int service);

void set_service(int service, bool enabled);

void run_services();

#endif
