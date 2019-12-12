SRCS=player_q.c player_q_a.c player_nn.c player_nn_a.c player_r.c game.c misc.c state.c
OBJS=$(addprefix out/, $(SRCS:.c=.o))
#OPT=-O3
#LTO=-flto
DEBUG=-g -fsanitize=address -fsanitize=undefined
CFLAGS=-I/usr/local/include -Wall $(LTO) $(OPT) $(DEBUG) -Wpedantic -Werror
LIBS=-L/usr/local/lib -ldoublefann
out/%.o: %.c game.h Makefile
	$(CC) $(CFLAGS) -c -o out/$*.o $*.c

krillin: $(OBJS)
	$(CC) $(LTO) $(OPT) $(OBJS) $(DEBUG) -o krillin $(LIBS)

clean:
	rm krillin
	rm $(OBJS)
