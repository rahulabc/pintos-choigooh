/* Ensures O(1) scheduling. */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/interrupt.h"
#include "devices/timer.h"
static thread_func simple_func;

void
test_alarm_choigooh (void) 
{
  /* Make sure our priority is the default. */
  ASSERT (thread_get_priority () == PRI_DEFAULT);

  int i, j, k;

  for(k=1; k<=512; k*=2)
  {
	  enum intr_level old_level = intr_disable();

	  num_add = 0;
	  num_remove = 0;
	  num_find = 0;
	  time_add = 0;
	  time_remove = 0;
	  time_find = 0;

	  /* create a number of threads */
	  for(i=0; i<k; i++)
	  {
		  char name[16];
		  snprintf(name, sizeof name, "Thread %d", i);
		  /* uniform priority */
		  thread_create(name, (PRI_DEFAULT+i)%PRI_MAX, simple_func, NULL);
	  }
	  intr_set_level(old_level);
	  
	  /* sufficient wait */
	  timer_sleep(2000);
	  msg("#ofThreads=%d : #of(Add,Remove,Find)=(%d,%d,%d), Totalticks=(%lld, %lld, %lld)", k, num_add, num_remove, num_find, time_add, time_remove, time_find);
  }

}

static void 
simple_func (void *aux UNUSED) 
{
  int i;
  
  for (i = 0; i < 5; i++) 
      thread_yield ();
}
