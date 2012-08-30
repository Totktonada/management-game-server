#ifndef ARGUMENTS_H_SENTRY
#define ARGUMENTS_H_SENTRY

#include <time.h>
#include "typedefs.h"

/* Exit status, if user type wrong command line arguments. */
#define ES_WRONG_ARGS 2

typedef struct arguments_t {
    const char *server_ip;
    uint server_port;
    int max_clients; /* -1 is without limitation */
    time_t time_before_round;
} arguments_t;

void process_arguments(arguments_t *arguments, const char **argv);

#endif /* ARGUMENTS_H_SENTRY */
