#include <list.h>
#include "devices/disk.h"

struct swap_slot
{
	int32_t index;
	struct list_elem elem;
};

struct list swap_table;

void swap_init();
int empty_slot_index(int32_t swap_slot_index);
int pointer_end();
int right_index(int32_t swap_slot_index);
void swap_in(void* kpage, int32_t swap_slot_index);
void swap_clear(int32_t swap_slot_index);
int32_t swap_out(void* kpage, int32_t swap_slot_index);

