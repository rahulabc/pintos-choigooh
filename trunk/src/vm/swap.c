#include "vm/swap.h"
#include "threads/malloc.h"
#include "devices/disk.h"

/* The swap slot whose index is higher than continuous_empty_slot is empty slot. */
int32_t continuous_empty_slot;
/* swap_max_slot is the maximum index number that swap slot can have. */
int32_t swap_max_slot;
struct disk *swap_disk;

/* init function. */
void swap_init()  {
	list_init(&swap_table);
	swap_disk = disk_get(1, 1);
	swap_max_slot = disk_size(swap_disk) / 8 - 1;
	continuous_empty_slot = 0;
}

/* if the swap slot is empty, return false. else return true. */
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
/* if swap slot is not empty, read the swap slot into memory. */

void swap_in(void* kpage, int32_t swap_slot_index) {
	ASSERT(!is_empty_slot(swap_slot_index));

	int i;
	for (i = 0 ; i < 8 ; i++)
		disk_read (swap_disk, swap_slot_index * 8 + i, kpage + (i * DISK_SECTOR_SIZE));
}

/* if swap slot is not empty, put the slot into the swap_table. i.e. clear the slot. */
void swap_clear(int32_t swap_slot_index) {
	ASSERT(!is_empty_slot(swap_slot_index));

	struct swap_slot* empty_slot = (struct swap_slot* )malloc(sizeof(struct swap_slot));
	empty_slot->index = swap_slot_index;
	list_push_front(&swap_table, &empty_slot->elem);
}

/* if swap slot index is -1, find the empty swap slot in swap_table, and write.
else write to the slot that the index points to. */
int32_t swap_out(void* kpage, int32_t swap_slot_index){
	ASSERT(swap_slot_index >= -1 && swap_slot_index <= swap_max_slot);
		//ASSERT(swap_slot_index == -1 || is_empty_slot(swap_slot_index));

	/* if swap slot is given, write to the slot. */
	if(swap_slot_index >= 0) {
		int i;
		for (i = 0 ; i< 8 ; i++)
			disk_write (swap_disk, swap_slot_index * 8 + i, kpage + (i * DISK_SECTOR_SIZE));
		return swap_slot_index;
	}
	/* find the empty slot and write. */
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
				disk_write(swap_disk, write_index * 8 + i, kpage + (i * DISK_SECTOR_SIZE)) ;
			return write_index;
		}
	}
}
