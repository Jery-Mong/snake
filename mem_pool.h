#ifndef __MEM_POOL_H
#define __MEM_POOL_H

struct mem_pool_struct {
	unsigned int item_max_quantum;
	int item_size;
	void **stack_end;
	unsigned int item_cnt;
	unsigned int malloc_cnt;
};
typedef struct mem_pool_struct mem_pool_t;

#define MAX_STACK_ITEM_NR 30

extern int mem_pool_init(mem_pool_t *);
extern void * item_pop(mem_pool_t *);
extern int  item_push(void  *, mem_pool_t *);
extern int count_stack_item(mem_pool_t *);
extern int count_total_item(mem_pool_t *);

#endif
