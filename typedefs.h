#ifndef TYPEDEFS_H_SENTRY
#define TYPEDEFS_H_SENTRY

typedef unsigned int uint;
typedef   signed int sint;

typedef enum command_kind {
    CMD_EMPTY,    /* '\n'            */
    CMD_HELP,     /* help [command]  */
    CMD_NICK,     /* nick [nick]     */
    CMD_CLIENTS,  /* clients         */
    CMD_PLAYERS,  /* players         */
    CMD_REQUESTS, /* requests        */
    CMD_MARKET,   /* market          */
    CMD_BUILD,    /* build count     */
    CMD_MAKE,     /* make count      */
    CMD_BUY,      /* buy count cost  */
    CMD_SELL,     /* sell count cost */
    CMD_TURN,     /* turn            */
    CMD_JOIN,     /* join            */
    CMD_WRONG,
    CMD_PROTOCOL_PARSE_ERROR
} command_kind;

#endif /* TYPEDEFS_H_SENTRY */
