#include <list.h>
#include <stdint.h>
#include <stdio.h>

void frame_table_init();
bool install_frame(void* upage, void* kpage);
void add_frame(void* kpage);
void remove_frame(void* kpage);
void destroy_frame(uint32_t * pd);
void* get_user_frame(bool zero);
void free_user_frame(void *kpage);
void unmap_user_frame(void *kpage);
void eviction();

struct frame
{
    void* upage;
    void* kpage;
    uint32_t* pd;
    struct list_elem elem;
};

struct list frame_table;

