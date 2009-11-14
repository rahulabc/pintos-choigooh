#include<lib/kernel/list.h>

struct list frame_table;
struct frame
{
    void * upage;
    void * kpage;
    uint32_t *pd;
    struct list_elem elem;
}

void init_frame_table(void)
void add_frame(const void *)
void remove_frame(struct frame *)
void * get_user_page(enum palloc_flags)
void free_user_page(void *kpage)
void eviction(void)
void * get_exist_page(const void *)
