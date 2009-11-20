#include "vm/swap.h"
#include "threads/malloc.h"
#include "devices/disk.h"

int32_t continuous_empty_slot;
int32_t swap_max_slot;
struct disk *swap_disk;

void swap_init()  {
	list_init(&swap_table);
	swap_disk = disk_get(1, 1);
	swap_max_slot = disk_size(swap_disk) / 8 - 1;
	continuous_empty_slot = 0;
}

bool is_empty_slot(int32_t swap_slot_index){
	if (swap_slot_index >= continuous_empty_slot && swap_slot_index <= swap_max_slot){
		return true;
	}
	
	struct list_elem *e;
	for (e = list_begin (&swap_table); e != list_end (&swap_table); e = list_next (e))   {
		struct swap_slot* s = list_entry (e, struct swap_slot, elem);
		if (s->index == swap_slot_index)
			return true;
	}
	return false;
}

void swap_in(void* kpage, int32_t swap_slot_index) {
	ASSERT(!is_empty_slot(swap_slot_index));
	
	int i;
	for (i = 0 ; i < 8 ; i++)
		disk_read (swap_disk, swap_slot_index * 8 + i, kpage + (i * DISK_SECTOR_SIZE));
}

void swap_clear(int32_t swap_slot_index) {
	ASSERT(!is_empty_slot(swap_slot_index));
	
	struct swap_slot* empty_slot = (struct swap_slot* )malloc(sizeof(struct swap_slot));
	empty_slot->index = swap_slot_index;
	list_push_front(&swap_table, &empty_slot->elem);
}

int32_t swap_out(void* kpage, int32_t swap_slot_index){
	ASSERT(swap_slot_index >= -1 && swap_slot_index <= swap_max_slot);
	ASSERT(swap_slot_index == -1 || is_empty_slot(swap_slot_index));
	
	if(swap_slot_index >= 0) {
		int i;
		for (i = 0 ; i< 8 ; i++)
			disk_write (swap_disk, swap_slot_index * 8 + i, kpage + (i * DISK_SECTOR_SIZE));
		return swap_slot_index;
	}
	else {
		if(list_empty(&swap_table)){
			if (continuous_empty_slot <= swap_max_slot) {
				int i;
				for (i = 0; i < 8; i++)
					disk_write(swap_disk, continuous_empty_slot * 8 + i, kpage + (i * DISK_SECTOR_SIZE));
				continuous_empty_slot++;
				return continuous_empty_slot-1;
			}
			else{
				return -1;
			}
		}
		else{
			struct list_elem *e = list_pop_front(&swap_table);
			struct swap_slot* write_slot = list_entry (e, struct swap_slot, elem);

			int32_t write_index = write_slot->index ;
			int i;
			for (i = 0; i < 8; i++)
				disk_write(swap_disk, write_index + i, kpage + (i * DISK_SECTOR_SIZE)) ;
			return write_index;
		}
	}
}
