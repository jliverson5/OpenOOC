/*
Copyright (c) 2016 Jeremy Iverson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


/* assert */
#include <assert.h>

/* jmp_buf, sigsetjmp, siglongjmp */
#include <setjmp.h>

/* printf */
#include <stdio.h>

/* EXIT_SUCCESS */
#include<stdlib.h>

/* mprotect, PROT_READ, PROT_WRITE, MAP_PRIVATE, MAP_ANONYMOUS, MAP_STACK */
#include <sys/mman.h>

/* CLOCK_MONOTONIC, struct timespec, clock_gettime */
#include <time.h>


#define NUM_ITERS  (1<<13) /* 8192 */
#define NUM_FIBERS (1<<10) /* 1024 */


/* The main context, i.e., the context which spawned all of the fibers. */
static jmp_buf S_main;

static long unsigned int S_kern=0;


static inline void
S_gettime(struct timespec * const t)
{
  struct timespec tt;
  clock_gettime(CLOCK_MONOTONIC, &tt);
  t->tv_sec = tt.tv_sec;
  t->tv_nsec = tt.tv_nsec;
}


static inline unsigned long
S_getelapsed(struct timespec const * const ts, struct timespec const * const te)
{
  struct timespec t;
  if (te->tv_nsec < ts->tv_nsec) {
    t.tv_nsec = 1000000000L + te->tv_nsec - ts->tv_nsec;
    t.tv_sec = te->tv_sec - 1 - ts->tv_sec;
  }
  else {
    t.tv_nsec = te->tv_nsec - ts->tv_nsec;
    t.tv_sec = te->tv_sec - ts->tv_sec;
  }
  return (unsigned long)(t.tv_sec * 1000000000L + t.tv_nsec);
}


static void
S_fiber_kern(void)
{
  S_kern++;

  /* Switch back to main context, so that a new fiber gets scheduled. */
  siglongjmp(S_main, 1);

  /* It is erroneous to reach this point. */
  abort();
}


int
main(void)
{
  unsigned int i, fid, num_fibers;
  unsigned long t_nsec;
  jmp_buf * fiber;
  struct timespec ts, te;

  num_fibers = NUM_FIBERS;

  /* Setup */
  fiber = malloc(num_fibers*sizeof(*fiber));
  assert(fiber);
  for (fid=0; fid<num_fibers; ++fid) {
    if (!sigsetjmp(fiber[fid], 1)) {
      /* This code is executed on the first call to sigsetjmp. */
    }
    else {
      /* This code is executed once siglongjmp is called. */
      S_fiber_kern();
    }
  }

  /* Testing */
  S_gettime(&ts);
  for (i=0; i<NUM_ITERS; ++i) {
    for (fid=0; fid<num_fibers; ++fid) {
      if (!sigsetjmp(S_main, 1)) {
        /* This code is executed on the first call to sigsetjmp. */
        siglongjmp(fiber[fid], 1);
      }
      else {
        /* This code is executed once siglongjmp is called. */
      }
    }
  }
  S_gettime(&te);
  t_nsec = S_getelapsed(&ts, &te);

  /* Teardown */
  free(fiber);

  /* Output results */
  printf("Total time (ns):   %lu\n", t_nsec);
  printf("Total swaps:       %u\n", NUM_ITERS*num_fibers*2);
  printf("Swap latency (ns): %.2f\n", (double)t_nsec/(NUM_ITERS*num_fibers*2));

  assert(S_kern == NUM_ITERS*num_fibers);

  return EXIT_SUCCESS;
}