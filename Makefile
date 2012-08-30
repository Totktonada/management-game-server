SRCMODULES = buffer.c lexer.c parser.c market.c msg_buffer.c messages.c \
			 commands.c arguments.c utils.c auctions.c end_month.c \
			 expire.c game.c notifications.c main.c
OBJMODULES = $(SRCMODULES:.c=.o)
HEADERS = $(SRCMODULES:.c=.h)
EXEC_FILE = management-game-server

# _POSIX_C_SOURCE=200112L for isblank();
# _BSD_SOURCE for inet_aton() and daemon().
DEFINE = -D_POSIX_C_SOURCE=200112L -D_BSD_SOURCE

# Invoke daemon().
#DEFINE = $(DEFINE) -DDAEMON

CFLAGS = -g -Wall -Wextra -ansi -pedantic $(DEFINE)
CFLAGS_MAIN = $(CFLAGS) -Wno-unused-parameter

default: $(EXEC_FILE)

main.o: main.c main.h
	$(CC) $(CFLAGS_MAIN) -c $< -o $@

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

$(EXEC_FILE): $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $@

ifneq (clean, $(MAKECMDGOALS))
ifneq (clang_analyze_clean, $(MAKECMDGOALS))
-include deps.mk
endif
endif

deps.mk: $(SRCMODULES) $(HEADERS)
	$(CC) -MM $^ > $@

clean:
	rm -f *.o $(EXEC_FILE) deps.mk *.core core

clang_analyze_clean:
	rm -f *.h.gch *.plist
