#ifndef EXPIRE_H_SENTRY
#define EXPIRE_H_SENTRY

#include <time.h>
#include "typedefs.h"

/* all -- time to next game round.
 * cur -- time to next warnings about it. */
typedef struct expire_t {
    time_t all;
    time_t cur;
} expire_t;

/* Do not produce any time events. */
void expire_stop(expire_t *expire);

/* If all less than dec, than all and cur set to zero and next
 * 'select' call returns by timeout. */
void expire_dec(expire_t *expire, uint dec);

/* Set timeout. */
void expire_set(expire_t *expire, time_t time);

#endif /* EXPIRE_H_SENTRY */
