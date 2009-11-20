#include "vm/swap.h"
#include "threads/malloc.h"
#define SWAP_MAX 4194304 -1
#include "devices/disk.h"

static int32_t empty_slot_pointer;

struct disk *disk;

void swap_init()  {
	list_init(&swap_table);
	empty_slot_pointer=0;
}

int empty_slot_index(int32_t swap_slot_index){
	if (swap_slot_index >= empty_slot_pointer && swap_slot_index <=SWAP_MAX){
		return 1;
	}
	struct list_elem *e;
	for (e = list_begin (&swap_table); e != list_end (&swap_table); e = list_next (e))   {
		struct swap_slot* s = list_entry (e, struct swap_slot, elem);
		int32_t index = s -> index ;
		if ( index == swap_slot_index)
			return 1;
	}
	return 0;
}

int pointer_end(){
	return (empty_slot_pointer==SWAP_MAX);
}

int right_index(int32_t swap_slot_index){
	if (swap_slot_index<0 || swap_slot_index>SWAP_MAX){
		return 0;
	}
	return 1;
}

void swap_in(void* kpage, int32_t swap_slot_index) {
	ASSERT (right_index (swap_slot_index)) ;
	ASSERT (!empty_slot_index (swap_slot_index)) ;
	disk = disk_get(1,1);
	int i;
	for (i = 0 ; i< 8 ; i++)
		disk_read (disk, swap_slot_index + i, kpage + (i * DISK_SECTOR_SIZE));
/*
	int size_swap_slot = sizeof(struct swap_slot);
	struct swap_slot* empty_slot = ( struct swap_slot* ) malloc(size_swap_slot);
	empty_slot->index = swap_slot_index;
	list_push_front(&swap_table, &empty_slot->elem);
*/

}

void swap_clear(int32_t swap_slot_index) {
	ASSERT (right_index (swap_slot_index)) ;
	ASSERT (!empty_slot_index (swap_slot_index)) ;
	int size_swap_slot = sizeof(struct swap_slot);
	struct swap_slot* empty_slot = ( struct swap_slot* ) malloc(size_swap_slot);
	empty_slot->index = swap_slot_index;
	list_push_front(&swap_table, &empty_slot->elem);
}

int32_t swap_out(void* kpage, int32_t swap_slot_index){
	ASSERT(swap_slot_index <= SWAP_MAX);
	disk = disk_get(1,1);
	if(swap_slot_index>=0) {
		int i;
		for (i = 0 ; i< 8 ; i++)
			disk_write (disk, swap_slot_index + i, kpage + (i * DISK_SECTOR_SIZE));
		return swap_slot_index;
	}
	else{
		if(list_empty(&swap_table)){
			if (!pointer_end()){
				int i;
				for (i = 0; i < 8; i++)
					disk_write(disk, empty_slot_pointer + i, kpage + (i * DISK_SECTOR_SIZE));
				empty_slot_pointer ++;
				return empty_slot_pointer-1;
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
				disk_write(disk, write_index + i, kpage + (i * DISK_SECTOR_SIZE)) ;
			return write_index;
		}
	}
}
