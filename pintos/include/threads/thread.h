#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include <stdlib.h>
#include "threads/interrupt.h"
#include "threads/synch.h"
#ifdef VM
#include "vm/vm.h"
#endif

struct lock;

/* States in a thread's life cycle. */
enum thread_status {
    THREAD_RUNNING, /* Running thread. */
    THREAD_READY,   /* Not running but ready to run. */
    THREAD_BLOCKED, /* Waiting for an event to trigger. */
    THREAD_DYING    /* About to be destroyed. */
};

/* States to represent how a thread exit. */

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) - 1) /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0      /* Lowest priority. */
#define PRI_DEFAULT 31 /* Default priority. */
#define PRI_MAX 63     /* Highest priority. */

/* A kernel thread or user process.
 *
 * Each thread structure is stored in its own 4 kB page.  The
 * thread structure itself sits at the very bottom of the page
 * (at offset 0).  The rest of the page is reserved for the
 * thread's kernel stack, which grows downward from the top of
 * the page (at offset 4 kB).  Here's an illustration:
 *
 *      4 kB +---------------------------------+
 *           |          kernel stack           |
 *           |                |                |
 *           |                |                |
 *           |                V                |
 *           |         grows downward          |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           +---------------------------------+
 *           |              magic              |
 *           |            intr_frame           |
 *           |                :                |
 *           |                :                |
 *           |               name              |
 *           |              status             |
 *      0 kB +---------------------------------+
 *
 * The upshot of this is twofold:
 *
 *    1. First, `struct thread' must not be allowed to grow too
 *       big.  If it does, then there will not be enough room for
 *       the kernel stack.  Our base `struct thread' is only a
 *       few bytes in size.  It probably should stay well under 1
 *       kB.
 *
 *    2. Second, kernel stacks must not be allowed to grow too
 *       large.  If a stack overflows, it will corrupt the thread
 *       state.  Thus, kernel functions should not allocate large
 *       structures or arrays as non-static local variables.  Use
 *       dynamic allocation with malloc() or palloc_get_page()
 *       instead.
 *
 * The first symptom of either of these problems will probably be
 * an assertion failure in thread_current(), which checks that
 * the `magic' member of the running thread's `struct thread' is
 * set to THREAD_MAGIC.  Stack overflow will normally change this
 * value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
 * the run queue (thread.c), or it can be an element in a
 * semaphore wait list (synch.c).  It can be used these two ways
 * only because they are mutually exclusive: only a thread in the
 * ready state is on the run queue, whereas only a thread in the
 * blocked state is on a semaphore wait list. */
struct record{
    struct list_elem elem;
    struct semaphore sema;

    tid_t tid;
    int exit_status;
};

struct thread {
    /* Owned by thread.c. */

    /* Shared between thread.c and synch.c. */
    struct record *record; /* Each Record from Child Process. */
    struct list child_list; /* Waiting list for children as a parent process */
    struct list_elem elem;  /* List element. */
    struct lock *wait_lock; /* 지금 기다리는 lock. */
    struct list held_lock;  /* 지금 가지고 있는 lock의 리스트. */
    struct list_elem all;   /* all list를 위한 list_elem */
    /* Owned by thread.c. */
    struct intr_frame tf; /* Information for switching */
    int64_t local_tick;   /* 깨어날 절대시간 */

#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint64_t *pml4; /* Page map level 4 */
#endif

    tid_t tid;          /* Thread identifier. */
    int priority;       /* Priority. */
    int local_priority; /* priority를 양보 받기 전의 priority */
    int nice;           /* mlfqs를 위한 필드 */
    int recent_cpu;     /* recent_cpu는 고정소수점으로 저장한다. */
    int exit_status;
    char name[16]; /* Name (for debugging purposes). */

    enum thread_status status; /* Thread state. */

#ifdef VM
    /* Table for whole virtual memory owned by thread. */
    struct supplemental_page_table spt;
#endif
    unsigned magic; /* Detects stack overflow. */
};

// 이미 thread 안에 list_elem 멤버를 하나 더 전용으로 만들었다고 치고.
// 그렇게 하면, 실제로 list에 넣는다고 하는게, 정확히는 list_push_back(list ,a->elem); 이렇게 되고, 그 안의 맨 마지막 list_elem에다가 list_entry()를 쓰면, 실제 자식 thread를 특정해서 뽑기가 가능.


/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init(void);
void thread_start(void);

void thread_tick(void);
void thread_print_stats(void);

typedef void thread_func(void *aux);
tid_t thread_create(const char *name, int priority, thread_func *, void *);

void thread_block(void);
void thread_unblock(struct thread *);

void thread_sleep(int64_t);
void thread_wakeup(int64_t);

void thread_reorder_ready(struct thread *t);

bool priority_more(const struct list_elem *a, const struct list_elem *b,
                   void *aux);
void thread_check_preempt(void);

struct thread *thread_current(void);
tid_t thread_tid(void);
const char *thread_name(void);

void thread_exit(void) NO_RETURN;
void thread_yield(void);

int thread_get_priority(void);
void thread_set_priority(int);

int thread_get_nice(void);
void thread_set_nice(int);
int thread_get_recent_cpu(void);
int thread_get_load_avg(void);

void thread_calc_recent_cpu(struct thread *t);
void recalc_load_avg(void);
void thread_calc_priority(struct thread *t);
void mlfqs_update_all(void);
void dirty_update(void);

void mlfqs_tick(int64_t ticks);

void do_iret(struct intr_frame *tf);

#endif /* threads/thread.h */
