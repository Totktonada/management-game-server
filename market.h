#ifndef MARKET_H_SENTRY
#define MARKET_H_SENTRY

#include "typedefs.h"

uint market_raw(uint cur_lvl, uint players_count);

uint market_raw_cost(uint cur_lvl);

uint market_prod(uint cur_lvl, uint players_count);

uint market_prod_cost(uint cur_lvl);

uint market_level_probability(uint cur_lvl, uint expected_lvl);

uint market_next_level(uint cur_lvl);

#endif /* MARKET_H_SENTRY */
