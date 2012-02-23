SRCMODULES = buffer.c lexer.c parser.c game.c parameters.c utils.c msg_buffer.c main.c
OBJMODULES = $(SRCMODULES:.c=.o)
HEADERS = $(SRCMODULES:.c=.h)
EXEC_FILE = management-game-server

DEFINE =
#DEFINE = -DDAEMON # invoke daemon ();
#DEFINE = -DDAEMON -DDAEMON_ALT # invoke daemon_alt (); see main.c.
CFLAGS = -g -Wall -ansi -pedantic $(DEFINE)

default: $(EXEC_FILE)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

$(EXEC_FILE): $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $@

ifneq (clean, $(MAKECMDGOALS))
-include deps.mk
endif

deps.mk: $(SRCMODULES) $(HEADERS)
	$(CC) -MM $^ > $@

clean:
	rm -f *.o $(EXEC_FILE) deps.mk *.core core
