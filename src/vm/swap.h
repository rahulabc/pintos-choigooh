#include <list.h>
#include "devices/disk.h"

struct swap_slot
{
	int32_t index;
	struct list_elem elem;
};

struct list swap_table;
