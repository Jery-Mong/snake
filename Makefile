cfile=snake.c mem_pool.c
CFLAGS=-std=c99 -Wall -lncurses -lpthread
final=snake

all:$(final)

snake:$(cfile:.c=.o)
	gcc $(CFLAGS) $^ -o $@
%.o:%.c
	gcc $(CFLAGS) -c  $< -o $@

clean:
	rm *.o $(final) 
