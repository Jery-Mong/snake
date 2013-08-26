#include <stdlib.h>
#include <string.h>
#include "mem_pool.h"

int mem_pool_init(mem_pool_t *pool)
{
	int i;

	if (pool->item_max_quantum < 2 || pool->item_size < 1)
		return -1;
	pool->stack_end = (void **)malloc(pool->item_max_quantum * sizeof(void *));
	
	for (i = 0; i < pool->item_max_quantum / 2; i++) {
		pool->stack_end[i] = malloc(pool->item_size);
		if (pool->stack_end[i] == NULL)
			goto err;
		memset(pool->stack_end[i], 0,pool->item_size);
	}
	
	pool->item_cnt = pool->item_max_quantum / 2;
	pool->malloc_cnt = pool->item_cnt;
	return 0;
	
err:
	for (i--; i  >= 0; i--) {
		free(pool->stack_end[i]);
	}
	return -1;
}
void* item_pop(mem_pool_t * pool)
{
	void *p;
	
	if (pool->item_cnt < 1)
		if (pool->malloc_cnt < pool->item_max_quantum) {
			p = (void *)malloc(pool->item_size);
			pool->malloc_cnt++;
			return p ? p : NULL;
		} else
			return NULL;
	
	pool->item_cnt--;
	return pool->stack_end[pool->item_cnt];
}
int item_push(void *p, mem_pool_t *pool)
{

	if (pool->item_cnt >= pool->item_max_quantum)
		return -1;
		
	pool->stack_end[pool->item_cnt] = p;
	pool->item_cnt++;
	return 0;
}
int count_stack_item(mem_pool_t *pool)
{
	return pool->item_cnt;
}
int count_malloc_item(mem_pool_t *pool)
{
	return pool->malloc_cnt;
}
