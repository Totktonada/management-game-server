#include "utils.h"

type_of_cmd get_cmd_type (char *str)
{
    if (STR_EQUAL_CASE_INS (str, "help")) {
        return CMD_HELP;
    } else if (STR_EQUAL_CASE_INS (str, "nick")) {
        return CMD_NICK;
    } else if (STR_EQUAL_CASE_INS (str, "status")) {
        return CMD_STATUS;
    } else if (STR_EQUAL_CASE_INS (str, "build")) {
        return CMD_BUILD;
    } else if (STR_EQUAL_CASE_INS (str, "prod")) {
        return CMD_PROD;
    } else if (STR_EQUAL_CASE_INS (str, "buy")) {
        return CMD_BUY;
    } else if (STR_EQUAL_CASE_INS (str, "sell")) {
        return CMD_SELL;
    } else if (STR_EQUAL_CASE_INS (str, "turn")) {
        return CMD_TURN;
    } else {
        return CMD_WRONG;
    }
}

/* Symbols write to buf from 0 position.
 * In end write '\0'.
 * (2^32-1) contain 10 symbols,
 * therefore for unsigned int range buf must
 * have size 11.
 * Returns:
 * size of string without '\0'. */
unsigned int number_to_str (char *buf, unsigned int number)
{
    unsigned int i = 0;
    unsigned int del = 1;

    while (number / 10 >= del) {
        del *= 10;
    }

    do {
        buf[i] = '0' + ((number / del) % 10);
        del /= 10;
        ++i;
    } while (del > 0);

    buf[i] = '\0';
    return i;
}

/* Get random number form 0 to max_value (inclusive). */
unsigned int get_random (unsigned int max_value)
{
    return (unsigned int) (max_value * rand ()
        / (RAND_MAX + 1.0));
}

/* Length of number string representation:
 * log10i (number) + 1 . */
unsigned int log10i (unsigned int number)
{
    unsigned int i = 0;
    unsigned int del = 1;

    while (number / 10 >= del) {
        ++i;
        del *= 10;
    }

    return i;
}
