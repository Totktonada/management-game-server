#ifndef AUCTIONS_H_SENTRY
#define AUCTIONS_H_SENTRY

#include "typedefs.h"
#include "game.h"

typedef enum request_type {
    REQUEST_RAW,
    REQUEST_PROD
} request_type;

void make_auction(game_t *game, request_type type, uint m_count);

#endif /* AUCTIONS_H_SENTRY */
