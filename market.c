#include "market.h"
#include "utils.h"

/* Static market data */
/* ================== */

/* In massive (implicatum_value * 2). */
const uint raw_per_player[5] =
    { 2, 3, 4, 5, 6 };
const uint prod_per_player[5] =
    { 6, 5, 4, 3, 2 };

const uint raw_cost[5] =
    { 800, 650, 500, 400, 300 };
const uint prod_cost[5] =
    { 6500, 6000, 5500, 5000, 4500 };

/* In massive (implicatum_value * 12). */
/* Sum by string is 12. */
const uint level_probability[5][5] = {
    { 4, 4, 2, 1, 1 },
    { 3, 4, 3, 1, 1 },
    { 1, 3, 4, 3, 1 },
    { 1, 1, 3, 4, 3 },
    { 1, 1, 2, 4, 4 }
};

/* Accessors to market data */
/* ======================== */

/* Calculate raw count for current market level.
 * This function used players_count for calculation,
 * therefore it returns correct value for current month
 * only before or after players to go bankrupt.
 * Rounding to lower number. */
uint market_raw(uint cur_lvl, uint players_count)
{
    return (uint) (players_count * raw_per_player[cur_lvl] / 2);
}

/* Minimum raw cost on market. */
uint market_raw_cost(uint cur_lvl)
{
    return raw_cost[cur_lvl];
}

/* Calculate prod need count for current market level.
 * This function used players_count for calculation,
 * therefore it returns correct value for current month
 * only before or after players to go bankrupt.
 * Rounding to lower number. */
uint market_prod(uint cur_lvl, uint players_count)
{
    return (uint) (players_count * prod_per_player[cur_lvl] / 2);
}

/* Maximum prod cost on market. */
uint market_prod_cost(uint cur_lvl)
{
    return prod_cost[cur_lvl];
}

/* Probability of some market level in procent. */
uint market_level_probability(uint cur_lvl, uint expected_lvl)
{
    return (uint) (level_probability[cur_lvl][expected_lvl] * 100 / 12);
}

/* ==== Algorithms ==== */
/* ==================== */

/* Return next market level. */
uint market_next_level(uint cur_lvl)
{
    /* Why 11? See comment to level_probability array. */
    int random = 1 + get_random(11); /* [1-12] */
    int sum = 0;
    int i;

    for (i = 0; i < 5; ++i) {
        sum += level_probability[cur_lvl][i];
        if (sum >= random) {
            return i;
        }
    }

    /* Not possible. */
    return cur_lvl;
}
