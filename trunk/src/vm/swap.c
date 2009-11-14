#include "vm/swap.h"
#include "threads/malloc.h"

void swap_init() 
{
	list_init(&swap_disk_list);
}

void swap_in(void* kpage, int32_t swap_slot_index)
{
	disk = disk_get(1,1);

	int i;	
	for (i = 0 ; i< 8 ; i++)
		disk_read (disk, swap_slot_index + i, kpage + (i * DISK_SECTOR_SIZE));
}

