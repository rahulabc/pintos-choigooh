#include <list.h>

struct sup_page
{
	void* kpage;
	void* upage;
	int32_t swap_slot_index;
	bool writable;
	bool swap_exist;
	struct thread* t;
	struct list_elem elem;
	
};

struct list sup_page_table;

