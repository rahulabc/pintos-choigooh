#include<stdio.h>
#include<frame.h>
#include"userprog/process.h"
#include"userprog/pagedir.h"
#include"threads/thread.h"
#include"threads/vaddr.h"

void init_frame_table()
{
    list_init(&frame_table);
}

void add_frame(const void *upage)
{
    struct thread *cur = thread_current();
    uint32_t *pd = cur->pagedir;
    struct frame new_frame = malloc(sizeof(struct frame));
    new_frame.pd = pd;
    new_frame.upage = upage;
    list_push_back(&frame_table, &new_frame.elem);
}

void remove_frame(struct frame *f)
{
    list_remove(&f->elem);
    free(f);
}

void destroy_frames(uint32_t *pd)
{
    struct list_elem *e;
    for(e=list_begin(&frame_table); e!=list_end(&frame_table); e=list_next(e))
    {
        struct frame *f = list_entry(e, struct frame, elem);
        if(f-pd==pd)
            remove_frame(f);
    }
}

void * get_user_page(enum palloc_flags flags)
{
    void * kpage = palloc_get_page(flags);
    add_frame(kpage);  // we have problem
    return kpage;
}

void free_user_page(void *kpage)
{
    struct list_elem *e;
    for(e=list_begin(&frame_table); e!=list_end(&frame_table); e=list_next(e))
    {
        struct frame *f = list_entry(e, struct frame, elem);
        /* if same with kpage */
            remove_frame(f);
        break;
    }
    palloc_free_page(kpage);
}

void eviction()
{
    int i, size;
    size = list_size(frame_table);
    i = 0;
    while(true)
    {
        i++;
        struct *frame = list_pop_front(&frame_table);
        if(pagedir_is_accessed(frame->pd, frame->upage))
        {
            pagedir_set_accessed(frame->pd, frame->upage, false);
            list_push_back(&frame_table, &frame->elem);
        }
        else
        {
            if(pagedir_is_dirty(frame->pd, frame->upage))
            {
                /* write back */

            }
           /* swap out */

           free(frame);
           break;
        }
    }
}

void * get_exist_page(const void *upage)
{
    void * new_kpage = palloc_get_page(PAL_USER);

    if(new_kpage!=NULL)
    {
        // swap in to new_kpage
        return new_kpage;
    }
    else
    {
        eviction();
        new_kpage = palloc_get_page(PAL_USER);
        if(new_kpage==NULL)
        {
            printf("Error !! \n");
            user_exit(-1);
        }
        return new_kpage;
    }
}
