#include "vm/page.h"
#include "threads/malloc.h"

void sup_page_table_init()
{
	list_init(&sup_page_table);
}

/* Add mapping and writable flag to the sup_page that has kpage and belongs to the current thread. */
bool install_sup_page(void* upage, void* kpage, bool writable)
{
  	struct list_elem *e;
  	for (e = list_begin (&sup_page_table); e != list_end (&sup_page_table); e = list_next (e))
  	{
		struct sup_page* p = list_entry (e, struct sup_page, elem);
		
		if (p->kpage == kpage && p->t == thread_current())
		{
			p->upage = upage;
			p->writable = writable;
			return true;
		}
  	}
    return false;
}

/* Add new sup_page to sup_page_table and initialize it */
void add_sup_page(void* kpage)
{
	struct sup_page* p = (struct sup_page*)malloc(sizeof(struct sup_page));
	p->t = thread_current();
	p->kpage = kpage;
	p->upage = NULL;
	p->swap_exist = false;

	list_push_back(&sup_page_table, &p->elem);
}

/* Find the sup_page which has kpage and change it's upage to NULL */
void unmap_sup_page(void* kpage)
{
	struct list_elem* e;
	for (e = list_begin (&sup_page_table); e != list_end (&sup_page_table); e = list_next (e))
  	{
		struct sup_page* p = list_entry (e, struct sup_page, elem);
		if (p->kpage == kpage)
		{
			p->kpage = NULL;
			return;
		}
  	}	
	ASSERT(false);
}

/* Remove the sup_page which has kpage from sup_page_table */
void remove_sup_page(void* kpage)
{
	struct list_elem* e;
	struct sup_page *p;
	for (e = list_begin (&sup_page_table); e != list_end (&sup_page_table); e = list_next (e))
  	{
		p = list_entry (e, struct sup_page, elem);
		if (p->kpage == kpage)
		{
			list_remove(&p->elem);
			if(p->swap_exist) // if swap exist, clear it from swap table
			{
				swap_clear(p->swap_slot_index);
			}
//			free(p);
			return;
		}
  	}	
	ASSERT(false);
}

/* Remove all the sup_pages that belong to the thread t. */
void destroy_sup_page(struct thread* t)
{
	struct list_elem* e;
	struct sup_page *p;
	for (e = list_begin (&sup_page_table); e != list_end (&sup_page_table); e = list_next (e))
  	{
		p = list_entry (e, struct sup_page, elem);
		if (p->t == t)
		{
			list_remove(&p->elem);
			if(p->swap_exist)	// if swap exist, clear it from swap_table
				swap_clear(p->swap_slot_index);
		}
  	}	
}

/* Find sup_page that has kpage. */
struct sup_page* get_sup_page_by_kpage(void* kpage)
{
	struct list_elem* e;
	for (e = list_begin (&sup_page_table); e != list_end (&sup_page_table); e = list_next (e))
  	{
		struct sup_page* p = list_entry (e, struct sup_page, elem);
		if (p->kpage == kpage)
			return p;
  	}	
	ASSERT(false);
	return NULL;
}

/* Find sup_page that has upage */
struct sup_page* get_sup_page_by_upage(void* upage)
{
	struct list_elem* e;
	for (e = list_begin (&sup_page_table); e != list_end (&sup_page_table); e = list_next (e))
  	{
		struct sup_page* p = list_entry (e, struct sup_page, elem);
		if (p->upage == upage)
			return p;
	}	
	return NULL;
}
