#include <stdlib.h>
#include <string.h>
#include "mem_pool.h"

int mem_pool_init(mem_pool_t *pool)
{
	int i;

	if (pool->item_max_quantum < 2 || pool->item_size < 1)
		return -1;
	
	pool->available = (void **)malloc(pool->item_max_quantum * sizeof(void *));
//	pool->used = (void **)malloc(pool->item_max_quantum * sizeof(void *));	
	
	for (i = 0; i < pool->item_max_quantum / 2; i++) {
		pool->available[i] = malloc(pool->item_size);
		if (pool->available[i] == NULL)
			goto err;
		memset(pool->available[i], 0,pool->item_size);
	}
	
	pool->avail_cnt = pool->item_max_quantum / 2;
	pool->total_malloc = pool->avail_cnt;
	return 0;
	
err:
	for (i--; i  >= 0; i--) {
		free(pool->available[i]);
	}
	return -1;
}
/*
void release_pool_mem(mem_pool_t *pool)
{
	int i;
}
 */
void* item_pop(mem_pool_t * pool)
{
	void *p;
	
	unsigned int used_cnt = pool->total_malloc - pool->avail_cnt;
	
	if (pool->avail_cnt < 1)
		if (pool->total_malloc < pool->item_max_quantum) {
			p = (void *)malloc(pool->item_size);
			pool->total_malloc++;
			//		pool->used[used_cnt] = p;
			return p ? p : NULL;
		} else
			return NULL;
	
	pool->avail_cnt--;
	//pool->used[used_cnt] = pool->available[pool->avail_cnt];
	return pool->available[pool->avail_cnt];
}
int item_push(void *p, mem_pool_t *pool)
{
	unsigned int used_cnt = pool->total_malloc - pool->avail_cnt;
	
	if (pool->avail_cnt >= pool->item_max_quantum)
		return -1;
		
	pool->available[pool->avail_cnt] = p;
	pool->avail_cnt++;
	return 0;
}
int count_stack_item(mem_pool_t *pool)
{
	return pool->avail_cnt;
}
int count_malloc_item(mem_pool_t *pool)
{
	return pool->total_malloc;
}
