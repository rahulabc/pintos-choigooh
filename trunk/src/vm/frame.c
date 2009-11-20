#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "vm/page.h"

void frame_table_init()
{
	list_init(&frame_table);
}

bool install_frame(void* upage, void* kpage)
{
  struct list_elem *e;
  struct frame *f;
  for (e = list_begin (&frame_table); e != list_end (&frame_table); e = list_next (e))
  {
		f = list_entry (e, struct frame, elem);
		
		if (f->kpage == kpage)
		{
			f->upage = upage;
            return true;
		}
  }
  return false;
}

void add_frame(void* kpage)
{
	struct frame* f = (struct frame*)malloc(sizeof(struct frame));
	f->kpage = kpage;
	f->pd = thread_current()->pagedir;
	list_push_back(&frame_table, &f->elem);
}

void remove_frame(void* kpage)
{
	struct list_elem* e;
	struct frame *f;

	for (e = list_begin (&frame_table); e != list_end (&frame_table); e = list_next (e))
    {
		f = list_entry (e, struct frame, elem);
		if (f->kpage == kpage)
		{
			list_remove(&f->elem);
			break;
		}
    }
	ASSERT(e==list_end(&frame_table));
}

void destroy_frame(uint32_t * pd)
{
	struct list_elem* e;
	struct frame *f;
	for (e = list_begin (&frame_table); e != list_end (&frame_table); e = list_next (e))
    {
		f = list_entry (e, struct frame, elem);
		if (f->pd == pd)
		{
			list_remove(&f->elem);
		}
    }
}

void* get_user_frame(bool zero)
{
	void* kpage = palloc_get_page(zero ? (PAL_USER | PAL_ZERO) : PAL_USER);

	if(kpage == NULL)
		kpage = eviction(zero);

	ASSERT(kpage != NULL);
	add_sup_page(kpage);
	add_frame(kpage);

	return kpage;
}

void free_user_frame(void *kpage)
{
    remove_frame(kpage);
    palloc_free_page(kpage);
    remove_sup_page(kpage);
}
void unmap_user_frame(void *kpage)
{
	remove_frame(kpage);
	unmap_sup_page(kpage);
	palloc_free_page(kpage);
}

void eviction(bool zero)
{
	void *val=NULL;
    do
    {
		struct list_elem *e = list_pop_front(&frame_table);
		struct frame *frame = list_entry(e, struct frame, elem);
        if(pagedir_is_accessed(frame->pd, frame->upage))
        {
            pagedir_set_accessed(frame->pd, frame->upage, false);
            list_push_back(&frame_table, &frame->elem);
        }
        else
        {
			struct sup_page *p = get_sup_page_by_kpage(frame->kpage);
			if(p->swap_exist)
			{
				ASSERT(swap_out(frame->kpage, p->swap_slot_index)==-1);
			}
			else
			{
				p->swap_slot_index = swap_out(frame->kpage, -1);
				p->swap_exist = true;
				ASSERT(p->swap_slot_index!=-1);
			}
			printf("evicted : %u %u\n", p->upage, p->kpage);
    		unmap_user_frame(frame->kpage);
			val = palloc_get_page(zero ? (PAL_USER | PAL_ZERO) : PAL_USER);
        }
    }while(val==NULL);
}

