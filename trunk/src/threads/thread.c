#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#ifdef USERPROG
#include "userprog/process.h"
#include "filesys/file.h"
#endif

/* Random value for struct thread's `magic' member.
   Used to detect stack overflow.  See the big comment at the top
   of thread.h for details. */
#define THREAD_MAGIC 0xcd6abf4b


/* Idle thread. */
static struct thread *idle_thread;

/* Initial thread, the thread running init.c:main(). */
static struct thread *initial_thread;

/* Lock used by allocate_tid(). */
static struct lock tid_lock;
/* Stack frame for kernel_thread(). */
struct kernel_thread_frame 
  {
    void *eip;                  /* Return address. */
    thread_func *function;      /* Function to call. */
    void *aux;                  /* Auxiliary data for function. */
  };

/* Statistics. */
static long long idle_ticks;    /* # of timer ticks spent idle. */
static long long kernel_ticks;  /* # of timer ticks in kernel threads. */
static long long user_ticks;    /* # of timer ticks in user programs. */

/* Scheduling. */
static unsigned thread_ticks;   /* # of timer ticks slice last yield. */
/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
bool thread_mlfqs;

int idle_flag;

static void kernel_thread (thread_func *, void *aux);

static void idle (void *aux UNUSED);
static struct thread *running_thread (void);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority);
static bool is_thread (struct thread *) UNUSED;
static void *alloc_frame (struct thread *, size_t size);
static void schedule (void);
void schedule_tail (struct thread *prev);
static tid_t allocate_tid (void);

void add_to_active_list(struct thread *);
void add_to_expired_list(struct thread *);
void clear_bit_from_active_bitmap(int priority);
void swap_active_and_expired(void);
struct thread *get_highest_priority_thread(void);

bool Sthread_comparator(const struct list_elem *a, const struct list_elem *b, void* aux UNUSED);
void thread_sleep(int64_t expiration);
void thread_wake_up(void);
bool thread_unblock_check_preemtion(struct thread *t);
void thread_unblock_with_preemption(struct thread *t);

/* Data structure and functions for ready_list and priority_bitmap */

const int OSMapTbl[] = {1, 2, 4, 8, 16, 32, 64, 128};
const int OSUnMapTbl[] = { 0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
	 												 4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4, 
													 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5, 
													 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5, 
													 6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6, 
													 6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6, 
													 6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6, 
													 6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6, 
													 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 
													 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 
													 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 
													 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 
													 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 
													 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 
													 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 
  												 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7 };

struct ready_list_tag
{
	struct list *active;
	struct list *expired;
	int *active_size;
	int *expired_size;
	int *active_tbl;
	int *expired_tbl;
	int active_grp;
	int expired_grp;
	int num_of_active;
	int num_of_expired;
	
	struct list arr1[PRI_MAX+1];
	struct list arr2[PRI_MAX+1];
	int arr1_size[PRI_MAX+1];
	int arr2_size[PRI_MAX+1];
	int OSRdyTbl1[PRI_MAX/8 + 1];
	int OSRdyTbl2[PRI_MAX/8 + 1];
} ready_list;


/* Data structure for timer_sleep() */
static struct list sleep_list;

struct Sthread
{
	struct thread *t;
	int64_t expiration_time;
  struct list_elem elem;
};


/* add thread t to active_list */
void add_to_active_list(struct thread *t)
{
	num_add++;
	int64_t start = timer_ticks();
	
	list_push_back(&ready_list.active[t->priority], &t->elem);
	
	//set bit on priority bitmap
	ready_list.active_grp |= OSMapTbl[t->priority >> 3];
	ready_list.active_tbl[t->priority >> 3] |= OSMapTbl[t->priority & 0x07];
	
	ready_list.num_of_active++;
	ready_list.active_size[t->priority]++;
	
	int64_t finish = timer_ticks();
	time_add += finish - start;
}

/* add thread t to expired_list */
void add_to_expired_list(struct thread *t)
{
	num_add++;
	int64_t start = timer_ticks();

	list_push_back(&ready_list.expired[t->priority], &t->elem);
	
	//set bit on priority bitmap
	ready_list.expired_grp |= OSMapTbl[t->priority >> 3];
	ready_list.expired_tbl[t->priority >> 3] |= OSMapTbl[t->priority & 0x07];
	
	ready_list.num_of_expired++;
	ready_list.expired_size[t->priority]++;

	int64_t finish = timer_ticks();
	time_add += finish - start;
}

void clear_bit_from_active_bitmap(int priority)
{
	if ((ready_list.active_tbl[priority >> 3] &= ~OSMapTbl[priority & 0x07]) == 0)
		ready_list.active_grp &= ~OSMapTbl[priority >> 3];
}

/* swap pointers and variables of active_list and expired_list */
void swap_active_and_expired(void)
{
	if(ready_list.active == ready_list.arr1)
	{
		ready_list.active = ready_list.arr2;
		ready_list.expired = ready_list.arr1;
		ready_list.active_tbl = ready_list.OSRdyTbl2;
		ready_list.expired_tbl = ready_list.OSRdyTbl1;
		ready_list.active_size = ready_list.arr2_size;
		ready_list.expired_size = ready_list.arr1_size;
	}
	else
	{
		ready_list.active = ready_list.arr1;
		ready_list.expired = ready_list.arr2;
		ready_list.active_tbl = ready_list.OSRdyTbl1;
		ready_list.expired_tbl = ready_list.OSRdyTbl2;
		ready_list.active_size = ready_list.arr1_size;
		ready_list.expired_size = ready_list.arr2_size;
	}
	int tmp = ready_list.active_grp;
	ready_list.active_grp = ready_list.expired_grp;
	ready_list.expired_grp = tmp;
	ready_list.num_of_active = ready_list.num_of_expired;
	ready_list.num_of_expired = 0;
}

/* find the highest priority in active_list and return the highest priority thread */
struct thread *get_highest_priority_thread(void)
{
	if(ready_list.num_of_active == 0 && ready_list.num_of_expired > 0)
		swap_active_and_expired(); 	// if active_list is empty and expired_list is not empty, swap active_list and expired_list.
	else if(ready_list.num_of_active == 0 && ready_list.num_of_expired == 0)
		return idle_thread;	// if active_list and expired_list are both empty, return idle_thread
		
	num_find++;
	int64_t start = timer_ticks();
	
	//find the highest priority using priority bitmap
	int y = OSUnMapTbl[ready_list.active_grp];
	int x = OSUnMapTbl[ready_list.active_tbl[y]];
	int highest_priority = (y << 3) + x;
	
	int64_t finish = timer_ticks();
	time_find += finish-start;
	
	num_remove++;
	start = timer_ticks();
	
	//if there's only one thread of that priority, clear the bit on bitmap
	if(ready_list.active_size[highest_priority] == 1)
		clear_bit_from_active_bitmap(highest_priority);
		
	ready_list.num_of_active--;
	ready_list.active_size[highest_priority]--;
	struct list_elem *temp = list_pop_front(&ready_list.active[highest_priority]);

	finish = timer_ticks();
	time_remove += finish - start;

	return list_entry(temp, struct thread, elem);
}


/* Initializes the threading system by transforming the code
   that's currently running into a thread.  This can't work in
   general and it is possible in this case only because loader.S
   was careful to put the bottom of the stack at a page boundary.

   Also initializes the run queue and the tid lock.

   After calling this function, be sure to initialize the page
   allocator before trying to create any threads with
   thread_create().

   It is not safe to call thread_current() until this function
   finishes. */
void
thread_init (void) 
{
  ASSERT (intr_get_level () == INTR_OFF);

  lock_init (&tid_lock);
  
	//initialize the struct ready_list
	int i;
	for(i = 0; i < PRI_MAX + 1; i++)
	{
		list_init(&ready_list.arr1[i]);
		list_init(&ready_list.arr2[i]);
		ready_list.arr1_size[i] = 0;
		ready_list.arr2_size[i] = 0;
	}
	
	for(i = 0; i < 8; i++)
	{
		ready_list.OSRdyTbl1[i] = 0;
		ready_list.OSRdyTbl2[i] = 0;
	}
	
	ready_list.active = ready_list.arr1;
	ready_list.expired = ready_list.arr2;
	ready_list.active_size = ready_list.arr1_size;
	ready_list.expired_size = ready_list.arr2_size;
	ready_list.active_tbl = ready_list.OSRdyTbl1;
	ready_list.expired_tbl = ready_list.OSRdyTbl2;
	ready_list.active_grp = 0;
	ready_list.expired_grp = 0;
	ready_list.num_of_active = 0;
	ready_list.num_of_expired = 0;
	
  list_init (&sleep_list);
  list_init (&process_table);
  list_init (&wait_list);
  /* Set up a thread structure for the running thread. */
  initial_thread = running_thread ();
  init_thread (initial_thread, "main", PRI_DEFAULT);
  initial_thread->status = THREAD_RUNNING;
  initial_thread->tid = allocate_tid ();
  list_init(&initial_thread->child_list);

  // put the initial thread to the process table
  list_push_front(&process_table, &initial_thread->table_elem);

}

/* Starts preemptive thread scheduling by enabling interrupts.
   Also creates the idle thread. */
void
thread_start (void) 
{
  /* Create the idle thread. */
  struct semaphore idle_started;
  sema_init (&idle_started, 0);
  thread_create ("idle", PRI_MIN, idle, &idle_started);

  /* Start preemptive thread scheduling. */
  intr_enable ();

  /* Wait for the idle thread to initialize idle_thread. */
  sema_down (&idle_started);
}

/* Called by the timer interrupt handler at each timer tick.
   Thus, this function runs in an external interrupt context. */
void
thread_tick (void) 
{
  struct thread *t = thread_current ();

  /* Update statistics. */
  if (t == idle_thread)
    idle_ticks++;
#ifdef USERPROG
  else if (t->pagedir != NULL)
    user_ticks++;
#endif
  else
    kernel_ticks++;

	//if the current_thread consumes all of its time_slice, yield CPU to next thread
 	if (++thread_ticks >= t->time_slice)
		intr_yield_on_return();
}

/* 	Sthread comparator for list_ordered_insert
	ascending order of expiration_time 	*/
bool Sthread_comparator(const struct list_elem *a, const struct list_elem *b, void* aux UNUSED)
{
	struct Sthread *c = list_entry(a, struct Sthread, elem);
	struct Sthread *d = list_entry(b, struct Sthread, elem);

	return c->expiration_time <= d->expiration_time;
}

/* implemation of the timer_sleep() */
void thread_sleep(int64_t expiration)
{
  struct thread *curr = thread_current();
  struct Sthread *s = malloc(sizeof(struct Sthread));
  enum intr_level old_level;

  old_level = intr_disable();
  s->t = curr;
  s->expiration_time = expiration;
  list_insert_ordered(&sleep_list, &s->elem, Sthread_comparator, (void *)NULL);
  thread_block();
  intr_set_level(old_level);
}

/* invoked by timer_interrupt 
 * wakes up sleeping threads */
void thread_wake_up(void)
{
	struct list_elem *e;
	int size = list_size(&sleep_list);

	// whether preempt or not
	bool preemption=false;
	
	if(size > 0)
	{
		struct thread *curr;
		struct Sthread *s;
		e = list_front(&sleep_list);
		s = list_entry(e, struct Sthread, elem);
		curr = s->t;

		while(timer_ticks() >= s->expiration_time)
		{
			/* wake up a thread */
			enum intr_level old_level = intr_disable();
			list_pop_front(&sleep_list);

			/* if there is at least one thread who have to preempt,
			 * preemption should be executed */
			preemption = thread_unblock_check_preemtion(curr) || preemption;
			intr_set_level(old_level);
			if(list_size(&sleep_list) == 0)
				break;
			e = list_front(&sleep_list);
			s = list_entry(e, struct Sthread, elem);
			curr = s->t;
		}
	}
	if(preemption)
		intr_yield_on_return();
}

/* Prints thread statistics. */
void
thread_print_stats (void) 
{
  printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
          idle_ticks, kernel_ticks, user_ticks);
}

/* Creates a new kernel thread named NAME with the given initial
   PRIORITY, which executes FUNCTION passing AUX as the argument,
   and adds it to the ready queue.  Returns the thread identifier
   for the new thread, or TID_ERROR if creation fails.

   If thread_start() has been called, then the new thread may be
   scheduled before thread_create() returns.  It could even exit
   before thread_create() returns.  Contrariwise, the original
   thread may run for any amount of time before the new thread is
   scheduled.  Use a semaphore or some other form of
   synchronization if you need to ensure ordering.

   The code provided sets the new thread's `priority' member to
   PRIORITY, but no actual priority scheduling is implemented.
   Priority scheduling is the goal of Problem 1-3. */
tid_t
thread_create (const char *name, int priority,
               thread_func *function, void *aux) 
{
  struct thread *t;
  struct kernel_thread_frame *kf;
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  tid_t tid;

  ASSERT (function != NULL);

  /* Allocate thread. */
  t = palloc_get_page (PAL_ZERO);
  if (t == NULL)
    return TID_ERROR;

  /* Initialize thread. */
  init_thread (t, name, priority);
  tid = t->tid = allocate_tid ();

  /* Stack frame for kernel_thread(). */
  kf = alloc_frame (t, sizeof *kf);
  kf->eip = NULL;
  kf->function = function;
  kf->aux = aux;

  /* Stack frame for switch_entry(). */
  ef = alloc_frame (t, sizeof *ef);
  ef->eip = (void (*) (void)) kernel_thread;

  /* Stack frame for switch_threads(). */
  sf = alloc_frame (t, sizeof *sf);
  sf->eip = switch_entry;

  /* Add to run queue. Add to the process table. */
  list_push_front(&process_table, &t->table_elem);
  thread_unblock_with_preemption(t);
  return tid;
}

/* Puts the current thread to sleep.  It will not be scheduled
   again until awoken by thread_unblock().

   This function must be called with interrupts turned off.  It
   is usually a better idea to use one of the synchronization
   primitives in synch.h. */
void
thread_block (void) 
{
  ASSERT (!intr_context ());
  ASSERT (intr_get_level () == INTR_OFF);
	
	struct thread *cur = thread_current();
	cur->status = THREAD_BLOCKED;
	cur->time_slice -= thread_ticks;	//update time_slice to hold the remaining time
	
  schedule ();
}

/* Transitions a blocked thread T to the ready-to-run state.
   This is an error if T is not blocked.  (Use thread_yield() to
   make the running thread ready.) */
void
thread_unblock (struct thread *t) 
{
	enum intr_level old_level;

	ASSERT (is_thread (t));
  old_level = intr_disable ();
  ASSERT (t->status == THREAD_BLOCKED);
	t->status = THREAD_READY;

	if(t != idle_thread)
		add_to_active_list(t);
		
  intr_set_level (old_level);
}

//unblock the thread. if preemption has to occur, current_thread yield the CPU
void thread_unblock_with_preemption(struct thread *t)
{
	if(thread_unblock_check_preemtion(t))
		thread_yield();
}

//unblock the thread and check whether preemption has to occur
bool thread_unblock_check_preemtion(struct thread *t)
{
	struct thread *curr = thread_current();
	thread_unblock(t);
	return t->priority > curr->priority;
}

/* Returns the name of the running thread. */
const char *
thread_name (void) 
{
  return thread_current ()->name;
}

/* Returns the running thread.
   This is running_thread() plus a couple of sanity checks.
   See the big comment at the top of thread.h for details. */
struct thread *
thread_current (void) 
{
  struct thread *t = running_thread ();
  
  /* Make sure T is really a thread.
     If either of these assertions fire, then your thread may
     have overflowed its stack.  Each thread has less than 4 kB
     of stack, so a few big automatic arrays or moderate
     recursion can cause stack overflow. */
  ASSERT (is_thread (t));
  ASSERT (t->status == THREAD_RUNNING);

  return t;
}

/* Returns the running thread's tid. */
tid_t
thread_tid (void) 
{
  return thread_current ()->tid;
}

/* Deschedules the current thread and destroys it.  Never
   returns to the caller. */
void
thread_exit (void) 
{
  ASSERT (!intr_context ());

#ifdef USERPROG
  process_exit ();
#endif

  /* Just set our status to dying and schedule another process.
     We will be destroyed during the call to schedule_tail(). */
  intr_disable ();
  thread_current ()->status = THREAD_DYING;
  schedule ();
  NOT_REACHED ();
}

/* Yields the CPU.  The current thread is not put to sleep and
   may be scheduled again immediately at the scheduler's whim. */
void
thread_yield () 
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;
  
  ASSERT (!intr_context ());

  old_level = intr_disable ();

	//if preemption occurs, add current_thread to active_list and update its time_slice
  if (cur != idle_thread && cur->time_slice > thread_ticks) 
	{
		cur->time_slice -= thread_ticks;
		add_to_active_list(cur);
	} //else if the current_thread just consumes all of its time_slice, add the thread to expired_list and recover its time_slice
	else if(cur != idle_thread && cur->time_slice <= thread_ticks)
	{
		cur->time_slice = cur->priority + 5;
		add_to_expired_list(cur);
	}
		
	cur->status = THREAD_READY;
	
	schedule ();
  intr_set_level (old_level);
}

/* Sets the current thread's priority to NEW_PRIORITY. */
void
thread_set_priority (int new_priority) 
{
  thread_current ()->priority = new_priority;
	thread_current ()->time_slice = new_priority + 5;
}

/* Returns the current thread's priority. */
int
thread_get_priority (void) 
{
  return thread_current ()->priority;
}

/* Sets the current thread's nice value to NICE. */
void
thread_set_nice (int nice UNUSED) 
{
  /* Not yet implemented. */
}

/* Returns the current thread's nice value. */
int
thread_get_nice (void) 
{
  /* Not yet implemented. */
  return 0;
}

/* Returns 100 times the system load average. */
int
thread_get_load_avg (void) 
{
  /* Not yet implemented. */
  return 0;
}

/* Returns 100 times the current thread's recent_cpu value. */
int
thread_get_recent_cpu (void) 
{
  /* Not yet implemented. */
  return 0;
}

/* Idle thread.  Executes when no other thread is ready to run.

   The idle thread is initially put on the ready list by
   thread_start().  It will be scheduled once initially, at which
   point it initializes idle_thread, "up"s the semaphore passed
   to it to enable thread_start() to continue, and immediately
   blocks.  After that, the idle thread never appears in the
   ready list.  It is returned by next_thread_to_run() as a
   special case when the ready list is empty. */
static void
idle (void *idle_started_ UNUSED) 
{
  struct semaphore *idle_started = idle_started_;
  idle_thread = thread_current ();
  sema_up (idle_started);

  for (;;) 
    {
      /* Let someone else run. */
      intr_disable ();
      thread_block ();

      /* Re-enable interrupts and wait for the next one.

         The `sti' instruction disables interrupts until the
         completion of the next instruction, so these two
         instructions are executed atomically.  This atomicity is
         important; otherwise, an interrupt could be handled
         between re-enabling interrupts and waiting for the next
         one to occur, wasting as much as one clock tick worth of
         time.

         See [IA32-v2a] "HLT", [IA32-v2b] "STI", and [IA32-v3a]
         7.11.1 "HLT Instruction". */
      asm volatile ("sti; hlt" : : : "memory");
    }
}

/* Function used as the basis for a kernel thread. */
static void
kernel_thread (thread_func *function, void *aux) 
{
  ASSERT (function != NULL);

  intr_enable ();       /* The scheduler runs with interrupts off. */
  function (aux);       /* Execute the thread function. */
  thread_exit ();       /* If function() returns, kill the thread. */
}

/* Returns the running thread. */
struct thread *
running_thread (void) 
{
  uint32_t *esp;

  /* Copy the CPU's stack pointer into `esp', and then round that
     down to the start of a page.  Because `struct thread' is
     always at the beginning of a page and the stack pointer is
     somewhere in the middle, this locates the curent thread. */
  asm ("mov %%esp, %0" : "=g" (esp));
  return pg_round_down (esp);
}

/* Returns true if T appears to point to a valid thread. */
static bool
is_thread (struct thread *t)
{
  return t != NULL && t->magic == THREAD_MAGIC;
}

/* Does basic initialization of T as a blocked thread named
   NAME. */
static void
init_thread (struct thread *t, const char *name, int priority)
{
  ASSERT (t != NULL);
  ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
  ASSERT (name != NULL);

  memset (t, 0, sizeof *t);
  t->status = THREAD_BLOCKED;
  strlcpy (t->name, name, sizeof t->name);
  t->stack = (uint8_t *) t + PGSIZE;
  t->esp = PHYS_BASE - PGSIZE;
  t->priority = priority;
	t->time_slice = priority + 5;
  t->magic = THREAD_MAGIC;
	list_init(&t->open_file_list);
	list_init(&t->child_list); 
	t->parent = NULL;
}

/* Allocates a SIZE-byte frame at the top of thread T's stack and
   returns a pointer to the frame's base. */
static void *
alloc_frame (struct thread *t, size_t size) 
{
  /* Stack data is always allocated in word-size units. */
  ASSERT (is_thread (t));
  ASSERT (size % sizeof (uint32_t) == 0);

  t->stack -= size;
  return t->stack;
}

/* Chooses and returns the next thread to be scheduled.  Should
   return a thread from the run queue, unless the run queue is
   empty.  (If the running thread can continue running, then it
   will be in the run queue.)  If the run queue is empty, return
   idle_thread. */
static struct thread *
next_thread_to_run (void) 
{
	return get_highest_priority_thread();
}

/* Completes a thread switch by activating the new thread's page
   tables, and, if the previous thread is dying, destroying it.

   At this function's invocation, we just switched from thread
   PREV, the new thread is already running, and interrupts are
   still disabled.  This function is normally invoked by
   thread_schedule() as its final action before returning, but
   the first time a thread is scheduled it is called by
   switch_entry() (see switch.S).

   It's not safe to call printf() until the thread switch is
   complete.  In practice that means that printf()s should be
   added at the end of the function.

   After this function and its caller returns, the thread switch
   is complete. */
void
schedule_tail (struct thread *prev) 
{
  struct thread *cur = running_thread ();
  
  ASSERT (intr_get_level () == INTR_OFF);

  /* Mark us as running. */
  cur->status = THREAD_RUNNING;

  /* Start new time slice. */
  thread_ticks = 0;

#ifdef USERPROG
  /* Activate the new address space. */
  process_activate ();
#endif

  /* If the thread we switched from is dying, destroy its struct
     thread.  This must happen late so that thread_exit() doesn't
     pull out the rug under itself.  (We don't free
     initial_thread because its memory was not obtained via
     palloc().) */
  if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread) 
    {
      ASSERT (prev != cur);
      palloc_free_page (prev);
    }
}

/* Schedules a new process.  At entry, interrupts must be off and
   the running process's state must have been changed from
   running to some other state.  This function finds another
   thread to run and switches to it.

   It's not safe to call printf() until schedule_tail() has
   completed. */
static void
schedule (void) 
{
//  struct thread *cur = running_thread ();
  struct thread *cur = running_thread ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));
  if (cur != next)
    prev = switch_threads (cur, next);
  schedule_tail (prev); 
}

/* Returns a tid to use for a new thread. */
static tid_t
allocate_tid (void) 
{
  static tid_t next_tid = 1;
  tid_t tid;

  lock_acquire (&tid_lock);
  tid = next_tid++;
  lock_release (&tid_lock);

  return tid;
}

/* Offset of `stack' member within `struct thread'.
   Used by switch.S, which can't figure it out on its own. */
uint32_t thread_stack_ofs = offsetof (struct thread, stack);
