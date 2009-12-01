#include <list.h>
#include "devices/disk.h"

struct cache_block
{
	int32_t sector_idx;
	void* data;
	bool present;
	bool accessed;
	bool dirty;
	struct list_elem elem;
};

struct list cache_list;