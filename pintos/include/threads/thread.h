#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/interrupt.h"
#ifdef VM
#include "vm/vm.h"
#endif

#ifdef USERPROG
struct child_status;
struct file;
#endif

void thread_wakeup(int64_t now_ticks); // sleep_list 순회 후 기다릴 시간이 지난 스레드 깨우는 함수

/* States in a thread's life cycle. */
enum thread_status {
	THREAD_RUNNING,     /* Running thread. */
	THREAD_READY,       /* Not running but ready to run. */
	THREAD_BLOCKED,     /* Waiting for an event to trigger. */
	THREAD_DYING        /* About to be destroyed. */
};

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

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
struct thread {
	int64_t wakeup_tick; /* 깨어날 tick을 저장할 필드 > alarm clock에서 사용 */

	/* Owned by thread.c. */
	tid_t tid;                          /* Thread identifier. */

	enum thread_status status;          /* Thread state. */
	/* status: 스레드의 상태입니다. 다음 중 하나를 가집니다:
	- `THREAD_RUNNING`
	스레드가 실행 중인 상태입니다. 한 시점에는 정확히 하나의 스레드만 실행됩니다. 
	`thread_current()`는 현재 실행 중인 스레드를 반환합니다.
	
	- `THREAD_READY`
	스레드는 실행할 준비가 되어 있지만 지금 당장은 실행 중이 아닙니다. 
	스케줄러가 다음에 호출될 때 선택될 수 있습니다. 
	준비 상태의 스레드는 `ready_list`라는 이중 연결 리스트에 보관됩니다.
	
	- `THREAD_BLOCKED`
	스레드는 어떤 것을 기다리고 있는 상태입니다. 
	예를 들어 락이 사용 가능해지거나 인터럽트가 발생하기를 기다릴 수 있습니다. 
	`thread_unblock()` 호출로 `THREAD_READY` 상태로 전이하기 전까지는 다시 스케줄되지 않습니다. 
	이는 보통 스레드를 자동으로 차단하고 깨워 주는 Pintos 동기화 프리미티브를 통해 간접적으로 처리하는 것이 가장 편합니다.
	차단된 스레드가 무엇을 기다리는지 *사전에* 알 수 있는 방법은 없지만, 백트레이스(backtrace)가 도움이 될 수 있습니다.
	
	- `THREAD_DYING`
	스케줄러가 다음 스레드로 전환한 뒤 이 스레드를 파괴합니다. 		*/

	char name[16];                      /* Name (for debugging purposes). */
	int priority;                       /* Priority. 범위는 PRI_MIN (0)부터 PRI_MAX (63)까지임. 우선순위 63이 가장 높다.*/

	/* Shared between thread.c and synch.c. */
	struct list_elem elem;              /* List element. */
	/* ready_list(실행 준비가 된 스레드 목록)나 sema_down()에서 세마포어를 기다리는 스레드 목록처럼,
	 * 이중 연결 리스트에 스레드를 넣기 위해 사용하는 "list element"입니다.
	 * 세마포어를 기다리는 스레드는 준비 상태가 아니고, 
	 * 그 반대도 마찬가지이므로 하나의 멤버로 두 역할을 겸할 수 있습니다. */

#ifdef USERPROG
	/* Owned by userprog/process.c. */
	uint64_t *pml4;                     /* Page map level 4 */
	int exit_status;
	struct list fd_table;              /* Per-process file descriptors. */
	int next_fd;                       /* Next fd number for regular files. */
	struct list children;              /* Direct child process states. */
	struct child_status *child_info;   /* State shared with this process's parent. */
	struct file *exec_file;            /* Executable kept open for write denial. */
#endif
#ifdef VM
	/* Table for whole virtual memory owned by thread. */
	struct supplemental_page_table spt;
#endif

	/* Owned by thread.c. */
	struct intr_frame tf;               /* Information for switching */
	unsigned magic;                     /* Detects stack overflow. */
};

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

void insert_sleep(struct thread *t);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

void do_iret (struct intr_frame *tf);

bool thread_priority_compare (const struct list_elem *a, const struct list_elem *b, void *aux);

#endif /* threads/thread.h */
