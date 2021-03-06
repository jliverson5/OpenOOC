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

/* SIGSEGV */
#include <errno.h>

/* uintptr_t */
#include <inttypes.h>

/* printf */
#include <stdio.h>

/* abort */
#include <stdlib.h>

/* mprotect, PROT_READ, PROT_WRITE */
#include <sys/mman.h>

/* ucontext_t */
#include <ucontext.h>

/* sysconf, _SC_PAGESIZE */
#include <unistd.h>

/* */
#include "src/ooc.h"


/* Maximum number of fibers per thread. */
#define OOC_NUM_FIBERS 10


/* State flags for an OOC fiber. */
enum ooc_state_flags
{
  OOC_RUNNING,
  OOC_WAITING,
  OOC_AIO_INPROGRESS
};


/* An execution context, henceforth known as a fiber. */
struct ooc_fiber
{
  unsigned int state;
  uintptr_t addr;
  ucontext_t uc;
};


/* The fibers. */
static __thread uintptr_t _addr[OOC_NUM_FIBERS];
static __thread ucontext_t _handler[OOC_NUM_FIBERS];
static __thread ucontext_t _trampoline[OOC_NUM_FIBERS];

/* Number of active fibers. */
static __thread int ooc_cur_fibers=1;

/* My fiber id. */
static __thread int _me=0;

#if 0
static void
_sigsegv_handler(void)
{
  int ret, i;
  size_t ps;
  uintptr_t addr;

  /* TODO Need to post an async-io request. */
  ooc_fiber[ooc_me].addr = (uintptr_t)_addr;

  /* TEMPORARY */
  ret = 0;

  if (OOC_AIO_INPROGRESS == ooc_fiber[ooc_me].state) {
    /* If this fiber has async-io in progress upon arriving here, then we need
     * to search the other existing fibers to see if any of their async-io has
     * completed. If not and if resources are available, then we can create a
     * new fiber to continue execution. If no resources are available, then we
     * must just wait until one of the existing fibers' async-io completes. */

    /* Find a fiber that is runnable */
    for (;;) {
      /* Search existing fibers for one whose async-io is finished. */
      for (i=0; i<ooc_cur_fibers; ++i) {
        if (OOC_AIO_INPROGRESS == ooc_fiber[i].state) {
          //ret = ooc_aio_error(&(ooc_fiber[i].aioreq));

          switch (ret) {
            case 0:
            /* TODO Need to check aio_return(&(ooc_fiber[i].aioreq)) to see
             * if all bytes were successfully read. */
            ooc_fiber[i].state = 0;
            goto OOC_SEARCH_DONE;

            case EINPROGRESS:
            break;

            case ECANCELED:
            default:
            /* TODO Arriving here is erroneous, act accordingly. */
            abort();
            break;
          }
        }
      }

      /* If we arrive here, then no runnable fiber was found. If there are
       * resources available for more fibers, then jump to OOC_NEW_FIBER label
       * and create a new fiber. Otherwise, jump to the OOC_SEARCH_AGAIN label
       * to search again. */
      if (ooc_cur_fibers < OOC_NUM_FIBERS) {
        goto OOC_NEW_FIBER;
      }
      goto OOC_SEARCH_AGAIN;

      /* If we arrive at this label it is because a runnable fiber was found
       * and we jumped out of the search to here. In this case, i holds the
       * id of the runnable fiber. Set ooc_me to i and break from the search so
       * that post-processing can be done for that fiber before returning to its
       * trampoline context. */
      OOC_SEARCH_DONE:
      ooc_me = i;
      break;

      /* If we arrive here, then no runnable fiber was found, but there are
       * enough resources to create a new fiber. To do this, we will simply
       * continue to the next iteration of the OOC_FOR loop. This has the effect
       * of creating a new fiber. */
      OOC_NEW_FIBER:
      /* TODO continue is not correct here!! */
      /* Change ooc_me value -- this is what actually makes me a new fiber. */
      //ooc_me = ooc_cur_fibers++;
      //continue;

      /* If we arrive here, then no runnable fiber was found and there are
       * not enough resources to create more. So, just go re-execute the
       * search for a runnable fiber. It is possible that by now, one of the
       * outstanding async-io requests has completed. */
      /* TODO We should be able to use aio_suspend somehow to do non-busy wait
       * for an async-io request to finish. */
      OOC_SEARCH_AGAIN:
      continue;
    }
  }

  /* If we arrive here, it is because a fiber was found (could be the original
   * fiber) that had no async-io in progress upon arriving here. In this case,
   * we should update that fiber's memory protections and switch back to its
   * trampoline context and allow its kernel handler to return. */

  /* Update memory protections for fiber's the pending address. */
  /* FIXME Should not be PROT_READ|PROT_WRITE unconditionally. */
  /* FIXME ps should be assigned once, not everytime this function is called. */
  ps = (size_t)sysconf(_SC_PAGESIZE);
  addr = (ooc_fiber[ooc_me].addr)&(~(uintptr_t)(ps-1));
  ret = mprotect((void*)addr, ps, PROT_READ|PROT_WRITE);
  assert(!ret);

  /* Switch to trampoline context. */
  setcontext(&(ooc_fiber[ooc_me].uc));
}
#endif


static void
_sigsegv_handler(void)
{
  int ret;
  size_t ps;
  uintptr_t addr;

  if (/* flags = none */0) {
    /* TODO Post an async-io request. */

    if (/* async-io has not finished */0) {
      ret = swapcontext(_main, _handler[_me]);
      assert(!ret);
    }

    /* Grant read protection to page containing offending address. */
    flag = PROT_READ;
  }
  else if (/* flags = read */0) {
    /* Grant write protection to page containing offending address. */
    flag = PROT_READ|PROT_WRITE;
  }
  else {
    /* error */
    abort();
  }

  /* Apply updates to page containing offending address. */
  /* FIXME ps should be assigned once, not everytime this function is called. */
  ps = (size_t)sysconf(_SC_PAGESIZE);
  addr = _addr[_me]&(~(uintptr_t)(ps-1)); /* page align */
  ret = mprotect((void*)addr, ps, PROT_READ|PROT_WRITE);
  assert(!ret);
}


void
ooc_sigsegv_trampoline(int const sig, siginfo_t * const si, void * const uc)
{
  /* Signal handler context. */
  static __thread ucontext_t _uc;
  /* Alternate stack for signal handler context. */
  static __thread char _stack[SIGSTKSZ];

  assert(SIGSEGV == sig);

  _addr[_me] = (uintptr_t)si->si_addr;

  getcontext(&_uc);
  _uc.uc_stack.ss_sp = _stack;
  _uc.uc_stack.ss_size = SIGSTKSZ;
  _uc.uc_stack.ss_flags = 0;
  sigemptyset(&(_uc.uc_sigmask));

  makecontext(&_uc, (void (*)(void))_sigsegv_handler, 0);

  swapcontext(&(_trampoline[_me]), &_uc);

  if (uc) {}
}


#ifdef TEST
/* assert */
#include <assert.h>

/* setjmp */
#include <setjmp.h>

/* sigaction */
#include <signal.h>

/* NULL, EXIT_SUCCESS, EXIT_FAILURE */
#include <stdlib.h>

/* memset */
#include <string.h>

/* mmap, munmap, PROT_NONE, MAP_PRIVATE, MAP_ANONYMOUS */
#include <sys/mman.h>

#define SEGV_ADDR (void*)(0x1)

int
main(int argc, char * argv[])
{
  int ret;
  struct sigaction act;
  char * mem;

  memset(&act, 0, sizeof(act));
  act.sa_sigaction = &ooc_sigsegv_trampoline;
  act.sa_flags = SA_SIGINFO;
  ret = sigaction(SIGSEGV, &act, NULL);
  assert(!ret);

  mem = mmap(NULL, 1<<16, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  assert(mem);

  mem[0] = (char)1; /* Raise a SIGSEGV. */
  assert(mem == _addr);

  mem[1] = (char)1; /* Should not raise SIGSEGV. */
  assert(mem == _addr);

  ret = munmap(mem, 1<<16);
  assert(!ret);

  return EXIT_SUCCESS;

  if (argc || argv) {}
}
#endif
