#include <list.h>
#include "devices/disk.h"
#include "filesys/cache.h"
#include "filesys/inode.h"
#include "filesys/file.h"
#include "threads/malloc.h"

void cache_init()
{
	list_init(&cache_list);
	int i;
	
	for(i=0; i<64; i++)
	{
		struct cache_block *c = malloc(sizeof(struct cache_block));
		c->present = false;
		c->sector_idx = NULL;
		c->accessed = false;
		c->dirty = false;
		c->data = malloc(DISK_SECTOR_SIZE);
		list_push_back(&cache_list, &c->elem);
	}
}

struct cache_block * get_cache_block(int32_t sector_idx)
{
	struct list_elem *e;
	for(e=list_begin(&cache_list); e!=list_end(&cache_list); e=list_next(e))
	{
		struct cache_block *c = list_entry(e, struct cache_block, elem);
		if(c->present && c->sector_idx==sector_idx)
			return c;
	}
	return NULL;
}

bool is_cache_exist(int32_t sector_idx)
{
	return get_cache_block(sector_idx)!=NULL;
}

int cache_size()
{
	int size=0;
	struct list_elem *e;
	for(e=list_begin(&cache_list); e!=list_end(&cache_list); e=list_next(e))
	{
		struct cache_block *c = list_entry(e, struct cache_block, elem);
		if(c->present)
			size++;
	}
	return size;
}

bool is_cache_full()
{
	return cache_size()==64;
}

void eviction()
{
    while(true)
    {
		struct list_elem *e = list_pop_front(&cache_list);
		struct cache_block *c = list_entry(e, struct cache_block, elem);
		ASSERT(c->present);
        if(c->accessed)
        {
			c->accessed = false;
            list_push_back(&cache_list, &cache_list);
        }
        else
        {
			if(c->dirty)
				disk_write(disk_get(0, 1), c->sector_idx, c->data);
			c->presnet = false;
        }
    }
}

