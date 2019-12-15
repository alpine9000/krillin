KANN=kann.c kautodiff.c
SRCS=player_q.c player_q_a.c player_nn.c player_nn_a.c player_r.c game.c misc.c state.c  nn_kann.c nn_fann.c $(KANN)
OBJS=$(addprefix out/, $(SRCS:.c=.o))
DEBUG_OBJS=$(addprefix out/debug/, $(SRCS:.c=.o))
OPT=-O3
#LTO=-flto
DEBUG=-g -fsanitize=address -fsanitize=undefined
CFLAGS=-I/usr/local/include -Wall -Wpedantic -Werror
LIBS=-L/usr/local/lib -lfloatfann
DEPS=game.h Makefile

out/%.o: kann/%.c Makefile
	$(CC) $(OPT) $(LTO) $(CFLAGS) -c -o out/$*.o kann/$*.c

out/%.o: %.c $(DEPS)
	$(CC) $(OPT) $(LTO) $(CFLAGS) -c -o out/$*.o $*.c

out/debug/%.o: kann/%.c Makefile
	$(CC) $(DEBUG) $(CFLAGS) -c -o out/debug/$*.o kann/$*.c

out/debug/%.o: %.c $(DEPS)
	$(CC) $(DEBUG) $(CFLAGS) -c -o out/debug/$*.o $*.c

all: krillin debug

krillin: $(OBJS)
	$(CC) $(LTO) $(OPT) $(OBJS) -o krillin $(LIBS)

debug: $(DEBUG_OBJS)
	$(CC) $(DEBUG) $(DEBUG_OBJS) -o debug $(LIBS)
clean:
	rm krillin
	rm -f $(OBJS) $(DEBUG_OBJS)
