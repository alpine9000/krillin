SRCS=player_q.c main.c
OBJS=$(addprefix out/, $(SRCS:.c=.o))
CFLAGS=-Wall -g -O3 #-fsanitize=address -fsanitize=undefined

out/%.o: %.c game.h Makefile
	$(CC) $(CFLAGS) -c -o out/$*.o $*.c

krillin: $(OBJS)
	$(CC) $(OBJS) -o krillin  -fsanitize=address -fsanitize=undefined -ldoublefann
