#include <unistd.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#include "snake.h"
#include "mem_pool.h"

#define UP_TO_DOWN -1
#define DOWN_TO_UP 1
#define LEFT_TO_RIGHT 2
#define RIGHT_TO_LEFT -2
#define OTHER 0



WINDOW * game_erea;
int real_x = 92;
int real_y = 28;


int g_speed = 100000;

struct segment *snake_head;
pthread_mutex_t sync_lock;
pthread_mutex_t run_lock;

#define APPLE_NR 3
struct node apple[APPLE_NR];

mem_pool_t snake_pool = {50, sizeof(struct segment), NULL, 0, 0};

#define SHOW(x, y, count) do {for (int i = 0; i < (count); i++) {	\
								wmove(game_erea,(y),(x));			\
								waddch(game_erea, ' ' | A_REVERSE);		\
								waddch(game_erea, ' ' | A_REVERSE);		\
							  }							\
						} while (0)

void paint_apple()
{
	int i;
	
	for (i = 0; i < APPLE_NR; i++) {
		wmove(game_erea, apple[i].y, apple[i].x);
		waddch(game_erea, 'A' | A_REVERSE);
	}
}

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
	sprintf(buf, "%d", count_node());
	wmove(game_erea, 0, 0);
	waddstr(game_erea, buf);
}
void display()
{
	wclear(game_erea);
	box(game_erea, '|', '=');
			
	pthread_mutex_lock(&sync_lock);
	paint_seg(snake_head);
	pthread_mutex_unlock(&sync_lock);
	paint_apple();
	
	paint_node_nr();
	wrefresh(game_erea);
}

void change_tail(struct segment **tailp)
{
	struct segment *tmp = *tailp;
	
	if (*tailp != snake_head) {
		*tailp = (*tailp)->pre;
		(*tailp)->next = NULL;
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
	
	if (hx > real_x || hy > real_y || hx < 1 || hy < 1)
		return -1;
	if (count_seg() < 4)
		return 0;
	p = ((p->next)->next)->next;	/* the snake's head would never hit his sencod or third segment */
	if (check_node_at_snake(p, hx, hy))
		return -1;
	return 0;
}

void update_apple(int index)
{
	int i;
	
	srand((unsigned int)time(NULL));
	if (index >=0 && index < APPLE_NR )    {
		apple[index].x = (unsigned int)(rand() % real_x) + 1;
		apple[index].y = (unsigned int)(rand() % real_y) + 1;
		
	} else 
		for (i = 0; i < APPLE_NR; i++) {
			apple[i].x = (unsigned int)(rand() % real_x) + 1;
			apple[i].y = (unsigned int)(rand() % real_y) + 1;
		}
}
int eat_apple()
{
	int i;
	for (i = 0; i < APPLE_NR; i++)  {
		if (snake_head->x == apple[i].x && snake_head->y == apple[i].y) {
			snake_head->count++;
			update_apple(i);
			
			g_speed -= g_speed * 0.01;

			return 1;
		}
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

	
	if (snake_head != tail) {
		if (tail->count == 0)
			change_tail(&tail);
		tail->count--;
		p->count++;
	}
	switch(p->dir) {
	case UP_TO_DOWN:
		p->y++;
		p->y = p->y + eat_apple();
		break;
	case DOWN_TO_UP:
		p->y--;
		p->y = p->y - eat_apple();
		break;
	case LEFT_TO_RIGHT:
		p->x++;
		p->x = p->x + eat_apple();
		break;
	case RIGHT_TO_LEFT :
		p->x--;
		p->x = p->x - eat_apple();
		break;
	}
	if (terminated_check())
		return -1;
	return 0;
}

void quit_game()
{
	//wclear(game_erea);
	wmove(game_erea, real_y / 2, real_x / 2);
	waddstr(game_erea, "Game Over\n");
	wrefresh(game_erea);
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
	
	switch (wgetch(game_erea)) {
	case 'a':
	case KEY_LEFT:
		key = RIGHT_TO_LEFT;
		break;

	case 'd':
	case KEY_RIGHT:
		key = LEFT_TO_RIGHT;
		break;

	case 'w':
	case KEY_UP:
		key = DOWN_TO_UP;
		break;

	case 's':
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
	
	keypad(game_erea, TRUE);
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
	
 	initscr();
	if (COLS < real_x + 2 ||  LINES  < real_y + 2) {
		move(0, 0);
		printw("To play this game, the window size request at least x : 94, y : 30,\n but this window size is x : %d, y : %d\n", COLS, LINES);
		getch();
		return 0;
	}
	
		
	game_erea = derwin(stdscr, real_y + 2, real_x + 2, (LINES - real_y - 2) / 5, (COLS - real_x - 2) / 2);

	pthread_mutex_init(&sync_lock, NULL);
	pthread_mutex_init(&run_lock, NULL);
	mem_pool_init(&snake_pool);
	noecho();
	
	snake_head = (struct segment *)item_pop(&snake_pool);

	if (NULL == snake_head)
		return -1;

	snake_head->x = 15;
	snake_head->y = 20;
	snake_head->dir = LEFT_TO_RIGHT;
	snake_head->count = 15;
	snake_head->pre = snake_head;
	snake_head->next = NULL;
	
	pthread_create(&tid, NULL, guide_snake, NULL);

	update_apple(-1);
	while(1) {
		try_pause();
		pthread_mutex_lock(&sync_lock);
		if(snake_move()) {
			pthread_cancel(tid);
			quit_game();
		}
		pthread_mutex_unlock(&sync_lock);
		display();

		if (snake_head->dir == LEFT_TO_RIGHT ||
			snake_head->dir == RIGHT_TO_LEFT)
			usleep(g_speed);
		else 
			usleep(g_speed * 2.5);

	}
	return 0;
}
