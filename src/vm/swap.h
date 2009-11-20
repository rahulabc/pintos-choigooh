#include <list.h>


struct swap_slot
{
	int32_t index;
	struct list_elem elem;
};

struct list swap_table;

void swap_init();
bool is_empty_slot(int32_t swap_slot_index);
void swap_in(void* kpage, int32_t swap_slot_index);
void swap_clear(int32_t swap_slot_index);
int32_t swap_out(void* kpage, int32_t swap_slot_index);
