#include <lib/kernel/list.h>

struct frame
{
    void* upage;
    void* kpage;
    uint32_t* pd;
    struct list_elem elem;
};

struct list frame_table;
