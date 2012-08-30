#include <time.h>

#include "expire.h"
#include "typedefs.h"
#include "utils.h"

/* In seconds. */
#define WARNINGS_PERIOD 10

/* Do not produce any time events. */
void expire_stop(expire_t *expire)
{
    expire->all = -1;
    expire->cur = -1;
}

/* If all less than dec, than all and cur set to zero and next
 * 'select' call returns by timeout. */
void expire_dec(expire_t *expire, uint dec)
{
    expire->all = MAX(expire->all - dec, 0);

    expire->cur = expire->all % WARNINGS_PERIOD;

    if (expire->all > 0 && expire->cur == 0) {
        expire->cur = WARNINGS_PERIOD;
    }
}

/* Set timeout. */
void expire_set(expire_t *expire, time_t time)
{
    expire->all = MAX(time, 0);
    expire->cur = MIN(expire->all, WARNINGS_PERIOD);
}
