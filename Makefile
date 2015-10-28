cfile=snake.c mem_pool.c
finial=snake
CFLAGS=-std=gnu99 -Wall -lncurses -lpthread

all:$(finial)

snake:$(cfile:.c=.o)
	gcc  $^ $(CFLAGS) -o $@

%.o:%.c
	gcc $(CFLAGS) -c $< -o $@
	
clean:
	rm -rf *.o $(finial)
