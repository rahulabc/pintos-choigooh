#include "vm/page.h"
#include "threads/malloc.h"

void sup_page_table_init()
{
	list_init(&sup_page_table);
}

void install_sup_page(void* upage, void* kpage, bool writable)
{
  struct list_elem *e;
  for (e = list_begin (&sup_page_table); e != list_end (&sup_page_table); e = list_next (e))
  {
		struct frame* p = list_entry (e, struct sup_page elem);
		
		if (p->kpage == kpage && p->t == thread_current())
		{
			p->upage = upage;
			p->writable = writable;
			break;
		}
  }
}

void add_sup_page(void* kpage)
{
	struct sup_page* p = (struct sup_page*)malloc(sizeof(struct sup_page));
	p->t = thread_current();
	p->kpage = kpage;
	p->upage = NULL;
	p->swap_exist = false;

	list_push_back(&sup_page_table, &p->elem);
}

void unmap_sup_page(void* kpage)
{
	struct list_elem* e;
	for (e = list_begin (&sup_page_table); e != list_end (&sup_page_table); e = list_next (e))
  {
		struct sup_page* p = list_entry (e, struct sup_page, elem);
		if (p->kpage == kpage)
		{
			p->upage = NULL;
			break;
		}
  }	
}

struct sup_page* get_sup_page_by_kpage(void* kpage)
{
	struct list_elem* e;
	for (e = list_begin (&sup_page_table); e != list_end (&sup_page_table); e = list_next (e))
  {
		struct sup_page* p = list_entry (e, struct sup_page, elem);
		if (p->kpage == kpage)
			return p;
  }	
	return NULL;
}

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