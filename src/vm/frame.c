#include <stdio.h>
#include <list.h>
#include <lock.h>
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "vm/page.h"

struct lock f_lock;

void frame_table_init()
{
	list_init(&frame_table);
    lock_init(&f_lock);
}

bool install_frame(void* upage, void* kpage)
{
  struct list_elem *e;
  lock_acquire(&f_lock);
  for (e = list_begin (&frame_table); e != list_end (&frame_table); e = list_next (e))
  {
		struct frame* f = list_entry (e, struct frame, elem);
		
		if (f->kpage == kpage)
		{
			f->upage = upage;
            lock_release(&f_lock);
            return true;
		}
  }
  lock_release(&f_lock);
  return false;
}

void add_frame(void* kpage)
{
	struct frame* f = (struct frame*)malloc(sizeof(struct frame));
	f->kpage = kpage;
	f->pd = thread_current()->pagedir;

    lock_acquire(&f_lock);
	list_push_back(&frame_table, &f->elem);
    lock_release(&f_lock);
}

void remove_frame(void* kpage)
{
	struct list_elem* e;

    lock_acquire(&f_lock);
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
    lock_release(&f_lock);
}

void destroy_frame(uint32_t * pd)
{
	struct list_elem* e;
    lock_acquire(&f_lock);
	for (e = list_begin (&frame_table); e != list_end (&frame_table); e = list_next (e))
    {
		struct frame* f = list_entry (e, struct frame, elem);
		if (f->pd == pd)
		{
			list_remove(&f->elem);
			free(f);
		}
    }
    lock_release(&f_lock);
}

void* get_user_frame(bool zero)
{
	void* kpage = (zero ? palloc_get_page(PAL_USER | PAL_ZERO) : palloc_get_page(PAL_USER));

	if(kpage == NULL)
	{
		eviction();
		kpage = (zero ? palloc_get_page(PAL_USER | PAL_ZERO) : palloc_get_page(PAL_USER));	
	}
	
	add_frame(kpage);
	add_sup_page(kpage);

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

void eviction()
{
    int i, size;
    lock_acquire(&f_lock);
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
				struct sup_page p* = get_user_frame_by_kpage(frame->kpage);
				if(p->swap_exist)
				{
					swap_out(frame->kpage, p->swap_slot_index);
				}
				else
				{
					p->swap_slot_index = swap_out(frame->kpage, -1);
					p->swap_exist = true;
				}
           }
           unmap_user_frame(frame->kpage);
           break;
        }
    }
    rock_release(&f_lock);
}

