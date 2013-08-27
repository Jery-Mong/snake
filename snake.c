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

#define RECORD_NR 5
struct record current;

WINDOW * game_erea;
int real_x = 92;
int real_y = 28;


struct segment *snake_head;
pthread_mutex_t sync_lock;
pthread_mutex_t run_lock;

#define APPLE_NR 2
struct node apple[APPLE_NR];

mem_pool_t snake_pool = {50, sizeof(struct segment), NULL, 0, 0};

#define SHOW(x, y, count) do{ for (int i = 0; i < (count); i++) {	\
			wmove(game_erea,(y),(x));			\
			waddch(game_erea, ' ');				\
		}							\
	} while (0)

void paint_apple()
{
	int i;
	
	for (i = 0; i < APPLE_NR; i++) {
		wmove(game_erea, apple[i].y, apple[i].x);
		waddch(game_erea, ' ');
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
	wmove(game_erea ,0 ,0);
	waddstr(game_erea, buf);
}
void display()
{
	wclear(game_erea);
	box(game_erea, '|', '=');
	wstandout(game_erea);
			
	pthread_mutex_lock(&sync_lock);
	paint_seg(snake_head);
	
	pthread_mutex_unlock(&sync_lock);
	paint_apple();
	wstandend(game_erea);
	
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
void show_result(struct record *p, int cnt)
{
	int i;
	
	for (i = 0; i < cnt; i++, p++) {
		wmove(game_erea, i, 0);
		
		if (p->cur == 1) {
			wstandout(game_erea);
			printw("%c %d %s %s", i + '0', p->score, p->name, p->date);
			wstandend(game_erea);
		}
		printw("%c %d %s %s", i + '0', p->score, p->name, p->date);
	}
}

void swap_record(struct record *r1, struct record *r1)
{
	struct record *tmp;

	memcpy(tmp, r1, sizeof(struct record));
	memcpy(r1, r2, sizeof(struct record));
	memcpy(r2, tmp, sizeof(struct record));
}
void save_and_show_result(struct record *p)
{
	int i, j;
	unsigned int cnt;
	struct record *record, *tmp;
	
	FILE *fd = fopen("snake_record.db", "r+");
	if (fd == NULL) {
		fd = fopen("snake_record.db", "w+");
		cnt = 1;
		fwrite(&cnt, sizeof(unsigned int), 1, fd);
		fwrite(p, sizeof(struct record), 1, fd); /* save last record in next position */
		fwrite(p, sizeof(struct record), 1, fd);
		show_result(record, 1);	
		return;
	}		

	fread(&cnt, sizeof(unsigned int), 1, fd); /* read the count of records which saved in the first position  */
	fwrite(p, sizeof(struct record), 1, fd); 


	record = (struct record*)malloc(sizeof(struct record) * cnt);
	fseek(fd, sizeof(unsigned int) + sizeof(struct record), SEEK_SET);
	fread(record, sizeof(struct record), cnt, fd);

	for (i = 0; i < cnt; i++)
		(record + i)->cur = 0; /* they are not the current record */
		
	if ((record + cnt - 1)->score < p->score) {
			
		memcpy(record + cnt - 1, p, sizeof(struct record));
		for (i = 0; i < cnt; i++)
			for (j = i + 1; j < cnt; i++)
				if ((record + i)->score < (record + j)->score) /* swap staff in two memory ereas */
					swap_record(record + j, record + i);
		
		fseek(fd, sizeof(unsigned int) + sizeof(struct record), SEEK_SET);
		fwrite(record, sizeof(struct record), cnt, fd);
	}

	show_result(record, cnt);
	free(record);
}
void quit_game()
{
	wmove(game_erea, real_y / 2, real_x / 2);
	current.score = count_node();
	
	save_and_show_result(&current);
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

int get_last_record(struct record *p)
{
	FILE *fd = fopen("snake_record.db","r");
	if (fd == NULL)
		return -1;
	fseek(fd, sizeof(unsigned int), SEEK_SET);
	if (fread(p, sizeof(struct record), 1, fd))
		return 0;
	else
		return -1;
}
void input_player_info()
{
	char name[32];
	time_t tt;

	struct record *p = &current;
	
	wmove(game_erea, 0, 0);
	if (! get_last_record(p)) {		
		wprintw(game_erea, "Your Name[default %s]:", p->name);
		wgetstr(game_erea, name);
		if (!strlen(name))
			strcpy(p->name, name);

	} else {
		while (1) {
			waddstr(game_erea, "Your Name:");
			wgetstr(game_erea, p->name);
			if (strlen(p->name))
				break;
		}
	}
	p->cur = 1;
	tt = time(NULL);
	strcpy(p->date, ctime(&tt));
	return;
}
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

	input_player_info();
	
	pthread_mutex_init(&sync_lock, NULL);
	pthread_mutex_init(&run_lock, NULL);
	mem_pool_init(&snake_pool);
	noecho();
	
	struct segment *tt = (struct segment *)item_pop(&snake_pool);
	tt->x = 15;
	tt->y = 20;
	tt->dir = LEFT_TO_RIGHT;
	tt->count = 15;
	tt->pre = tt;
	tt->next = NULL;
	
	snake_head = tt;
	
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
		usleep(100000);
	}
	return 0;
}
