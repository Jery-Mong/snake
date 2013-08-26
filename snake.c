#include <unistd.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "snake.h"
#include "mem_pool.h"


#define UP_TO_DOWN -1
#define DOWN_TO_UP 1
#define LEFT_TO_RIGHT 2
#define RIGHT_TO_LEFT -2
#define OTHER 0

struct segment *snake_head;
pthread_mutex_t sync_lock;
pthread_mutex_t run_lock;
struct node apple;
mem_pool_t snake_pool = {50, sizeof(struct segment), NULL, 0, 0};

#define SHOW(x, y, count) do{ for (int i = 0; i < (count); i++) {	\
			move((y),(x));					\
			addch(' ');					\
		}							\
	} while (0)

#define paint_apple() {standout(); move(apple.y, apple.x); addch(' '); standend();}

void paint_seg(struct segment *p)
{
	if (p == NULL)
		return;
	if ( p->count == 0 && p != snake_head)
		return;
	
	int x = p->x;
	int y = p->y;
	
	switch (p->dir) {
	case UP_TO_DOWN:
		SHOW(x, y--, p->count);
		break;
	case DOWN_TO_UP:
		SHOW(x, y++, p->count);
		break;
	case LEFT_TO_RIGHT:
		SHOW(x--, y,p->count);
		break;
	case RIGHT_TO_LEFT:
		SHOW(x++, y, p->count);
		break;	
	}	
	paint_seg(p->next);	
}
int ntoa(unsigned int num, char *buf)	
{
	buf[0] = num / 1000 + '0';
	buf[1] = (num / 100) % 10 + '0';
	buf[2] = (num / 10) % 10 + '0';
	buf[3] = num % 10 + '0';
	buf[4] = '\0';
	return 0;	
}

#define for_each_segment(iter, head) for(iter = head; iter; iter = iter->next)

unsigned int count_seg()
{
	struct segment *p = snake_head;
	int i = 0;
	for_each_segment(p, snake_head)
		i++;
	return i;
}
unsigned int count_node()
{
	struct segment *p;
	unsigned int cnt = 0;
	
	for_each_segment(p, snake_head)
		cnt += p->count;
	return cnt;
}
void paint_node_nr()
{
	char buf[10] = {0};
	ntoa(count_node(), buf);
	move(0,0);
	addstr(buf);
}
#define display() do{					\
		clear();				\
		pthread_mutex_lock(&sync_lock);		\
		standout();				\
		paint_seg(snake_head);			\
		standend();				\
		pthread_mutex_unlock(&sync_lock);	\
		paint_apple();				\
		paint_node_nr();			\
		refresh();				\
	} while(0)

void change_tail(struct segment **tailp)
{
	struct segment *tmp = *tailp;
	
	if (*tailp != snake_head) {
		*tailp = (*tailp)->pre;
		(*tailp)->next = NULL;
		//	free(tmp);
		item_push(tmp, &snake_pool);
			
	}
}
int check_node_at_snake(struct segment *p, unsigned int hx, unsigned int hy)
{	
	while (p) {
		switch (p->dir) {
		case UP_TO_DOWN:
			if (hx == p->x )
				if (hy <= p->y && hy >= (p->y - p->count))
					return 1;
			break;
		case DOWN_TO_UP:
			if (hx == p->x)
				if (hy >= p->y && hy <= (p->y + p->count))
					return 1;
			break;
		case LEFT_TO_RIGHT:
			if (hy == p->y)
				if (hx <= p->x && hx >= (p->x - p->count))
					return 1;
			break;
		case RIGHT_TO_LEFT:
			if (hy == p->y)
				if (hx >= p->x && hx <= (p->x + p->count))
					return 1;
			break;
		}
		p = p->next;
	}
	return 0;
}
int terminated_check()
{
	struct segment *p = snake_head;
	int hx = p->x;
	int hy = p->y;
	
	if (hx >= COLS || hy >= LINES || hx < 0 || hy < 0)
		return -1;
	if (count_seg() < 4)
		return 0;
	p = ((p->next)->next)->next;	/* the snake's head would never hit his sencod or third segment */
	if (check_node_at_snake(p, hx, hy))
		return -1;
	return 0;
}

void update_apple()
{
	srand((unsigned int)time(NULL));
	apple.x = (unsigned int)(rand() % COLS);
	apple.y = (unsigned int)(rand() % LINES);
}
int eat_apple(struct segment *p)
{
	if (p->x == apple.x && p->y == apple.y) {
		p->count++;
		update_apple();
		return 1;
	}
	return 0;
}
int snake_move()
{
	struct segment *p = snake_head;
	struct segment *tail = NULL;
	
	tail = snake_head;
	while (tail->next)
		tail = tail->next;

	if (terminated_check())
		return -1;
	
	if (snake_head != tail) {
		if (tail->count == 0)
			change_tail(&tail);
		tail->count--;
		p->count++;
	}
	switch(p->dir) {
	case UP_TO_DOWN:
		p->y++;
		p->y = p->y + eat_apple(p);
		break;
	case DOWN_TO_UP:
		p->y--;
		p->y = p->y - eat_apple(p);
		break;
	case LEFT_TO_RIGHT:
		p->x++;
		p->x = p->x + eat_apple(p);
		break;
	case RIGHT_TO_LEFT :
		p->x--;
		p->x = p->x - eat_apple(p);
		break;
	}
	return 0;
}
void quit_game()
{
	move(LINES /2,COLS /2);
	addstr("Game Over");
	refresh();
	getch();
	endwin();
	exit(0);
}

void insert_head(struct segment *p)
{
	if (p == NULL)
		return;
	
	struct segment *tmp = snake_head;
	p->next = tmp;
	p->pre = p;
	tmp->pre = p;
	snake_head = p;
}

struct segment * new_segment(int key_dir)
{
	struct segment *old = snake_head;

	if (old->dir == key_dir || (old->dir + key_dir) == 0 )
		return NULL;
	if (key_dir == OTHER)
		return NULL;
	
	struct segment *new = (struct segment *)item_pop(&snake_pool);
	if (new == NULL)
		return NULL;
	new->x = old->x;
	new->y = old->y;
	new->count = 0;
	new->dir = key_dir;

	return new;
}
int get_arrow_key(void)
{
	int key;
	
	noecho();
	
	switch (getch()) {
	case KEY_LEFT:
		key = RIGHT_TO_LEFT;
		break;
	case KEY_RIGHT:
		key = LEFT_TO_RIGHT;
		break;
	case KEY_UP:
		key = DOWN_TO_UP;
		break;
	case KEY_DOWN:
		key = UP_TO_DOWN;
		break;
	case ' ':		/* Backspace */
		key = ' ';
		break;
	default:
		key = OTHER;
		break;
	}
	echo();
	return key;
}
void resume_and_pause_snake()
{
	if (pthread_mutex_trylock(&run_lock))
		pthread_mutex_unlock(&run_lock);
}
void *guide_snake(void * data)
{
	int key;
	struct segment *new;
	
	keypad(stdscr, TRUE);
	while (1) {
		key = get_arrow_key();
		if (snake_head->count == 0)
			continue;
		
		switch (key){
		case UP_TO_DOWN:
		case DOWN_TO_UP:
		case LEFT_TO_RIGHT:
		case RIGHT_TO_LEFT:
			pthread_mutex_lock(&sync_lock);
			new = new_segment(key);
			insert_head(new);
			pthread_mutex_unlock(&sync_lock);
			break;
		case ' ':
			resume_and_pause_snake();
			break;
		default:
			break;	
		}
	}
}

#define try_pause() {pthread_mutex_lock(&run_lock);pthread_mutex_unlock(&run_lock);}

int main(void)
{
	pthread_t tid;
	pthread_mutex_init(&sync_lock, NULL);
	pthread_mutex_init(&run_lock, NULL);

	mem_pool_init(&snake_pool);
	
	struct segment *tt = (struct segment *)item_pop(&snake_pool);
	tt->x = 10;
	tt->y = 20;
	tt->dir = UP_TO_DOWN;
	tt->count = 20;
	tt->pre = tt;
	tt->next = NULL;
	
	snake_head = tt;
	
	initscr();
	pthread_create(&tid, NULL, guide_snake, NULL);

	update_apple();
	while(1) {
		try_pause();
		pthread_mutex_lock(&sync_lock);
		if(snake_move()) {
			pthread_cancel(tid);
			quit_game();
		}
		pthread_mutex_unlock(&sync_lock);
		display();
		usleep(100000);
	}
	return 0;
}
