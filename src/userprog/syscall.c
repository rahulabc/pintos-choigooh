#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/init.h"
#include "userprog/process.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "devices/input.h"
#include "lib/user/syscall.h"
#include "threads/synch.h"
#include "vm/page.h"
#include "threads/vaddr.h"

static struct lock file_lock;

static void syscall_handler (struct intr_frame *);
static int get_user (const uint8_t *);
static bool put_user (uint8_t *, uint8_t);

void
syscall_init (void) 
{
  	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
	lock_init(&file_lock);
}

static void
syscall_handler (struct intr_frame *f) 
{
	int syscall_num = *(int*)(f->esp);
	int fd;
	struct file *file_;
	struct thread * cur = thread_current();
	off_t size;
	char* buf;
	
	switch(syscall_num)
	{
		case SYS_HALT:
			power_off();
			break;

		case SYS_EXIT:
			process__exit(*(int*)(f->esp + 4));
            // print exit message
			printf("%s: exit(%d)\n", cur->name, *(int*)(f->esp + 4));
            thread_exit();
			break;

		case SYS_EXEC:
			f->eax = process_execute(*(char **)(f->esp + 4));
			break;

		case SYS_WAIT:
			f->eax = process_wait(*(tid_t *)(f->esp + 4));
			break;

		case SYS_CREATE:
			// if file_name is null or initial_size < 0 then exit(-1)
			if(*(char**)(f->esp + 4) == NULL) user_exit(-1);
			if(*(off_t*)(f->esp + 8) < 0) user_exit(-1);
			lock_acquire(&file_lock);
			f->eax = filesys_create(*(char**)(f->esp + 4), *(off_t*)(f->esp + 8));
			lock_release(&file_lock);
			break;

		case SYS_REMOVE:
			// if file_name is null then exit(-1)
			if(*(char**)(f->esp + 4) == NULL) user_exit(-1);
			lock_acquire(&file_lock);
			filesys_remove(*(char**)(f->esp + 4));
			lock_release(&file_lock);
			break;

		case SYS_OPEN:
			// if file_name is null or file cannot be opened then exit(-1)
			if(*(char**)(f->esp + 4) == NULL) user_exit(-1);
			lock_acquire(&file_lock);
			file_ = (struct file*)filesys_open(*(char**)(f->esp + 4));
			if(file_ == NULL) 
			{
				lock_release(&file_lock);
				user_exit(-1);
			}
			file_->fd = fd_cnt++;	// assign file descriptor
			struct thread *t = thread_current();
			list_push_back(&open_file_table, &file_->table_elem);	// add this file to open_file_table
			list_push_back(&t->open_file_list, &file_->list_elem);// add this file to the process's open_file_list
			f->eax = file_->fd;
			lock_release(&file_lock);
			break;

		case SYS_FILESIZE:
			// if fd < 3 then exit(-1)
			if(*(int*)(f->esp + 4) < 3) user_exit(-1);
			// get file from open_file_list of current thread
			file_ = get_file(*(int*)(f->esp + 4));
			if(file_ == NULL) user_exit(-1);
			lock_acquire(&file_lock);
			f->eax = file_length(file_);
			lock_release(&file_lock);
			break;

		case SYS_READ:
			fd = *(int*)(f->esp + 4);
			buf = *(char**)(f->esp + 8);
			size = *(off_t*)(f->esp + 12);
			
			if(buf == NULL || size < 0) user_exit(-1);
			
			if(fd == 0)	// if STDIN
				f->eax = input_getc();
			else if(fd == 1 || fd == 2)
				f->eax = -1;
			else if(fd > 2)
			{
				file_ = get_file(fd);
				if(file_ == NULL) user_exit(-1);
				
				lock_acquire(&file_lock);
				int i;
				// check page_fault
				for(i = 0; i < size; i++)
				{
					if(buf + i >= PHYS_BASE)
						user_exit(-1);
					
					if(!put_user(buf + i, 0))
						user_exit(-1);
				}
				f->eax = file_read(file_, buf, size);
				lock_release(&file_lock);
			}
			break;

		case SYS_WRITE:
			fd = *(int*)(f->esp + 4);
			buf = *(char**)(f->esp + 8);
			size = *(off_t*)(f->esp + 12);
			
			if(buf == NULL || size < 0) user_exit(-1);
			
			if(fd == 1)	// if STDOUT
			{	
				putbuf(buf, size);
				f->eax = *(int*)(f->esp + 12);
			}
			else if(fd == 0 || fd == 2)
				f->eax = -1;
			else if(fd > 2)
			{
				file_ = get_file(fd);
				if(file_ == NULL) user_exit(-1);
				lock_acquire(&file_lock);
				int i;
				//check page fault
				for(i = 0; i < size; i++)
				{
					if(buf + i >= PHYS_BASE)
						user_exit(-1);
					
					if(!get_user(buf + i))
						user_exit(-1);
				}
				f->eax = file_write(file_, *(char**)(f->esp + 8), *(off_t*)(f->esp + 12));
				lock_release(&file_lock);
			}
			break;

		case SYS_SEEK:
			if(*(int*)(f->esp + 4) < 3) user_exit(-1);
			file_ = get_file(*(int*)(f->esp + 4));
			if(file_ == NULL) user_exit(-1);
			lock_acquire(&file_lock);
			file_seek(file_, *(off_t*)(f->esp + 8));
			lock_release(&file_lock);
			break;

		case SYS_TELL:
			if(*(int*)(f->esp + 4) < 3) user_exit(-1);
			file_ = get_file(*(int*)(f->esp + 4));
			if(file_ == NULL) user_exit(-1);
			lock_acquire(&file_lock);
			f->eax = file_tell(file_);
			lock_release(&file_lock);
			break;

		case SYS_CLOSE:
			if(*(int*)(f->esp + 4) < 3) user_exit(-1);
			file_ = get_file(*(int*)(f->esp + 4));
			if(file_ == NULL) user_exit(-1);
			
			lock_acquire(&file_lock);
			list_remove(&file_->list_elem);
			list_remove(&file_->table_elem);
			file_close(file_);
			lock_release(&file_lock);
			break;
	}
}


/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */
static int
get_user (const uint8_t *uaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}

/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
       : "=&a" (error_code), "=m" (*udst) : "r" (byte));
  return error_code != -1;
}

