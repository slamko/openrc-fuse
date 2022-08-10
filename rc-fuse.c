#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <rc.h>

int main(int argc, char **argv) {
    RC_STRINGLIST *active_services = rc_services_in_state(RC_SERVICE_STARTED);
    for (struct rc_string *service = active_services->tqh_first;
         service;
         service = service->entries.tqe_next) {
        printf("name: %s\n", service->value);
    }
    return !!active_services;
}

