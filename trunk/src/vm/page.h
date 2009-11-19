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

void sup_page_table_init();
bool install_sup_page(void* upage, void* kpage, bool writable);
void add_sup_page(void* kpage);
void unmap_sup_page(void* kpage);
void remove_sup_page(void* kpage);
void destroy_sup_page(struct thread* t);
struct sup_page* get_sup_page_by_kpage(void* kpage);
struct sup_page* get_sup_page_by_upage(void* upage);
