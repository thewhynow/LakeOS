#include <kernel/srv.h>
#include <kernel/kmm.h>

#include <stddef.h>

service_t *services;
size_t services_num;

int register_service(void (*service_f)(void *args), void *args){
    services = krealloc (
        services,
        (services_num + 1) * sizeof(service_t)
    );

    services[services_num] = (service_t){
        .service_f = service_f,
        .args      = args,
        .enabled   = true
    };

    return services_num++;
}

service_t *get_service(int service){
    return services + service;
}

void set_service(int service, bool enabled){
    services[service].enabled = enabled;
}

void run_services(){
    for (size_t i = 0; i < services_num; ++i)
        if (services[i].enabled)
            services[i].service_f(services[i].args);
}
