/* Shuffles a 128 kB data buffer 10 times, printing the checksum
   after each time. */

#include <stdbool.h>
#include "tests/arc4.h"
#include "tests/cksum.h"
#include "tests/lib.h"
#include "tests/main.h"

#define SIZE (1024 * 1024)

static char buf1[SIZE];
static char buf2[SIZE];
static char buf3[SIZE];
static char buf4[SIZE];
static char buf5[SIZE];

void swap(int *a, int *b)
{
	int t = *a;
	*a = *b;
	*b = t;
}

void result(int test)
{
	int j;
	int r1=0, r2=0, r3=0, r4=0, r5=0;
	for (j = 0; j < SIZE; j++)
	{
		r1 += buf1[j];
		r2 += buf2[j];
		r3 += buf3[j];
		r4 += buf4[j];
		r5 += buf5[j];
	}
	msg ("<%d> buf1 : %d, buf2 : %d, buf3 : %d, buf4 : %d, buf5 : %d", test, r1, r2, r3, r4, r5);
}
void
test_main (void)
{
  	size_t i, j;
	int r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0;
	
  /* Initialize. */
	memset(buf1, 1, SIZE);
	memset(buf2, 2, SIZE);
	memset(buf3, 3, SIZE);
	memset(buf4, 4, SIZE);
	memset(buf5, 5, SIZE);

	msg ("Initially,  buf1 : %d, buf2 : %d, buf3 : %d, buf4 : %d, buf5 : %d", SIZE, SIZE*2, SIZE*3, SIZE*4, SIZE*5);

	msg("<Shuffle test>");
  	for (i = 0; i < 10; i++)
    {
      	shuffle (buf1, SIZE, 1);
		shuffle (buf2, SIZE, 1);
		shuffle (buf3, SIZE, 1);
		shuffle (buf4, SIZE, 1);
		shuffle (buf5, SIZE, 1);
		result(i);
    }

	for(i=0; i<100; i++)
	{
		swap(buf1[i], buf2[i]);
		swap(buf3[i], buf4[i]);
	}
	msg("<Swap test>");
	result(1);
}
