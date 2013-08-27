#ifndef __SNAKE_H
#define __SNAKE_H

struct segment {
	unsigned int x, y;
	int dir;
	unsigned int count;
	struct segment *pre;
	struct segment *next;
};
struct node {
	unsigned int x;
	unsigned int y;
};
struct record {
	char name[32];
	unsigned int score;
	char date[32];
	int cur;
};
#endif
