#include <stdio.h>
#include <list.h>
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "vm/frame.h"
#include "vm/swap.h"

void frame_table_init()
{
	list_init(&frame_table);
}

void install_frame(void* upage, void* kpage, uint32_t* pd)
{
  struct list_elem *e;
  for (e = list_begin (&frame_table); e != list_end (&frame_table); e = list_next (e))
  {
		struct frame* f = list_entry (e, struct frame, elem);
		
		if (f->kpage == kpage)
		{
			f->upage = upage;
			f->pd = pd;
		}
  }
}

void add_frame(void* kpage)
{
	struct frame* f = (struct frame*)malloc(sizeof(struct frame));
	f->kpage = kpage;
	//f->upage = ptov(kpage);
	f->pd = thread_current()->pagedir;
	
	list_push_back(&frame_table, &f->elem);
}

void remove_frame(void* kpage)
{
	struct list_elem* e;
	for (e = list_begin (&frame_table); e != list_end (&frame_table); e = list_next (e))
  {
		struct frame* f = list_entry (e, struct frame, elem);
		if (f->kpage == kpage)
		{
			list_remove(&f->elem);
			free(f);
			break;
		}
  }
}

void* get_user_frame(enum palloc_flags flags)
{
	void* kpage = palloc_get_page(flags);

	if(kpage == NULL)
	{
		eviction();
		kpage = palloc_get_page(flags);	
	}
	
//	install_page(kpage, ptov(kpage), pt에서 얻어와 );

	add_frame(kpage);
	add_sup_page(kpage);

	return kpage;
}

void free_user_frame(void *kpage)
{
	remove_frame(kpage);
	unmap_sup_page(kpage);
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

