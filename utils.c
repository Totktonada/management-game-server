#include "utils.h"

type_of_cmd get_cmd_type (char *str)
{
    if (STR_EQUAL_CASE_INS (str, "help")) {
        return CMD_HELP;
    } else if (STR_EQUAL_CASE_INS (str, "nick")) {
        return CMD_NICK;
    } else if (STR_EQUAL_CASE_INS (str, "status")) {
        return CMD_STATUS;
    } else if (STR_EQUAL_CASE_INS (str, "prod")) {
        return CMD_PROD;
    } else if (STR_EQUAL_CASE_INS (str, "buy")) {
        return CMD_BUY;
    } else if (STR_EQUAL_CASE_INS (str, "sell")) {
        return CMD_SELL;
    } else if (STR_EQUAL_CASE_INS (str, "build")) {
        return CMD_BUILD;
    } else if (STR_EQUAL_CASE_INS (str, "turn")) {
        return CMD_TURN;
    } else {
        return CMD_WRONG;
    }
}
