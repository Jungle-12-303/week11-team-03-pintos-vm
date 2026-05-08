#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/tss.h"
#include "userprog/syscall.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/mmu.h"
#include "threads/vaddr.h"
#include "devices/timer.h"
#include "intrinsic.h"
#ifdef VM
#include "vm/vm.h"
#endif

static void process_cleanup (void);
static bool load (char *file_name, struct intr_frame *if_);
static void initd (void *aux);
static void __do_fork (void *);

// thread fork를 위한 구조체
struct fork_aux {
	struct thread *parent;
	struct intr_frame parent_if;
	struct child_status *child_info;
};

// fn_copy와 child_status *를 함께 넘기기 위한 작은 aux 구조체
struct initd_aux {
	char *file_name;
	struct child_status *child_info;
};

/* 프로세스별 fd table의 한 항목.
   fd는 user program이 사용하는 정수 file descriptor이고,
   file은 실제 열린 파일 객체를 가리킨다.
   elem은 thread_current()->fd_table 리스트에 연결하기 위한 list element다. */
struct fd_entry {
	int fd;
	struct file *file;
	struct list_elem elem;
};

static bool process_list_initialized (struct list *list);
static struct fd_entry *process_find_fd_entry (struct thread *t, int fd);
static void process_close_files (struct thread *t);
static void process_close_exec_file (struct thread *t);

/* General process initializer for initd and other process. */
static void
process_init (void) {
	struct thread *current = thread_current ();
	process_user_init (current);
}

void
process_user_init (struct thread *t) {
	ASSERT (t != NULL);

	if (!process_list_initialized (&t->fd_table))
		list_init (&t->fd_table);
	if (!process_list_initialized (&t->children))
		list_init (&t->children);
	if (t->next_fd < 2)
		t->next_fd = 2;
}

/* thread 구조체 안의 list 필드가 list_init()을 거쳤는지 확인한다.
   초기화되지 않은 fd_table/children에 list 연산을 수행하지 않기 위한 방어 helper다. */
static bool
process_list_initialized (struct list *list) {
	return list->head.next != NULL && list->tail.prev != NULL;
}

int
process_add_file (struct file *file) {
	struct thread *cur = thread_current ();
	struct fd_entry *entry;

	if (file == NULL)
		return -1;

	/* fd 0(stdin), fd 1(stdout)은 syscall 쪽에서 특별 취급하므로
	   일반 파일 fd는 현재 프로세스의 next_fd 값인 2부터 할당한다. */
	process_user_init (cur);
	entry = malloc (sizeof *entry);
	if (entry == NULL)
		return -1;

	entry->fd = cur->next_fd++;
	entry->file = file;
	list_push_back (&cur->fd_table, &entry->elem);
	return entry->fd;
}

static struct fd_entry *
process_find_fd_entry (struct thread *t, int fd) {
	struct list_elem *e;

	if (t == NULL || !process_list_initialized (&t->fd_table))
		return NULL;

	for (e = list_begin (&t->fd_table); e != list_end (&t->fd_table);
	     e = list_next (e)) {
		struct fd_entry *entry = list_entry (e, struct fd_entry, elem);
		if (entry->fd == fd)
			return entry;
	}
	return NULL;
}

struct file *
process_get_file (int fd) {
	/* 현재 프로세스 fd table에서 일반 파일 fd에 대응하는 struct file을 찾는다.
	   stdin/stdout 같은 표준 fd 처리는 syscall 계층에서 분기한다. */
	struct fd_entry *entry = process_find_fd_entry (thread_current (), fd);
	return entry != NULL ? entry->file : NULL;
}

bool
process_close_file (int fd) {
	struct fd_entry *entry = process_find_fd_entry (thread_current (), fd);

	if (entry == NULL)
		return false;

	/* fd table에서 제거한 뒤 file을 닫아 이 프로세스가 가진 fd 소유권을 끝낸다. */
	list_remove (&entry->elem);
	file_close (entry->file);
	free (entry);
	return true;
}

void
process_close_all_files (void) {
	process_close_files (thread_current ());
}

static void
process_close_files (struct thread *t) {
	if (t == NULL || !process_list_initialized (&t->fd_table))
		return;

	/* 프로세스 종료 시 명시적으로 close하지 않은 fd들을 모두 정리한다. */
	while (!list_empty (&t->fd_table)) {
		struct list_elem *e = list_pop_front (&t->fd_table);
		struct fd_entry *entry = list_entry (e, struct fd_entry, elem);
		file_close (entry->file);
		free (entry);
	}
}

bool
process_duplicate_fds (struct thread *dst, struct thread *src) {
	struct list_elem *e;

	ASSERT (dst != NULL);
	ASSERT (src != NULL);

	/* fork된 child는 parent와 같은 fd 번호를 유지해야 한다.
	   file_duplicate()은 file 위치와 deny_write 상태를 포함한 새 file 객체를 만든다. */
	process_user_init (dst);
	if (!process_list_initialized (&src->fd_table))
		return true;

	dst->next_fd = src->next_fd;
	for (e = list_begin (&src->fd_table); e != list_end (&src->fd_table);
	     e = list_next (e)) {
		struct fd_entry *src_entry = list_entry (e, struct fd_entry, elem);
		struct fd_entry *dst_entry = malloc (sizeof *dst_entry);
		if (dst_entry == NULL)
			goto fail;

		dst_entry->fd = src_entry->fd;
		dst_entry->file = file_duplicate (src_entry->file);
		if (dst_entry->file == NULL) {
			free (dst_entry);
			goto fail;
		}
		list_push_back (&dst->fd_table, &dst_entry->elem);
	}
	return true;

fail:
	process_close_files (dst);
	return false;
}

struct child_status *
process_find_child (tid_t tid) {
	struct thread *cur = thread_current ();
	struct list_elem *e;

	if (!process_list_initialized (&cur->children))
		return NULL;

	/* wait()는 직접 자식에게만 허용되므로 현재 thread의 children 목록만 탐색한다. */
	for (e = list_begin (&cur->children); e != list_end (&cur->children);
	     e = list_next (e)) {
		struct child_status *cs = list_entry (e, struct child_status, elem);
		if (cs->tid == tid)
			return cs;
	}
	return NULL;
}

void
child_status_release (struct child_status *cs) {
	if (cs == NULL)
		return;

	/* child_status는 부모와 자식이 함께 참조한다.
	   부모가 wait하거나 종료할 때, 자식이 exit할 때 각각 참조를 내려 0이 되면 해제한다. */
	cs->ref_cnt--;
	if (cs->ref_cnt <= 0)
		free (cs);
}

/* 실행 파일에 걸어 둔 write deny를 해제하고 파일을 닫는다.
   process_exec()에서 새 실행 파일로 교체할 때와 process_exit()에서 종료할 때
   같은 정리 흐름을 재사용하기 위한 helper 함수.
   exec_file은 rox를 위해 load 성공 후 닫지 않고 보관한 실행 파일이다. */
static void
process_close_exec_file (struct thread *t) {
	if (t == NULL || t->exec_file == NULL) {
		return;
	}

	lock_acquire (&filesys_lock);
	file_allow_write (t->exec_file);
	file_close (t->exec_file);
	lock_release (&filesys_lock);

	t->exec_file = NULL;
}

// 현재 프로세스가 부모로서 들고 있던 자식 목록을 비우고, 각 자식 상태에 대한 부모측의 참조를 해제하는 헬퍼 함수
static void
process_release_children (struct thread *t) { // t는 종료 중인 현재 thread
	if (t == NULL || !process_list_initialized (&t->children))
		return;

	/* 부모가 먼저 종료되면 더 이상 wait할 수 없으므로 parent-side 참조를 모두 내려놓는다.
	   자식이 아직 살아 있으면 자식 쪽 참조가 남아 있어 child_status는 유지된다. */
	while (!list_empty (&t->children)) {
		struct list_elem *e = list_pop_front (&t->children); // children 리스트에서 앞에서부터 제거
		struct child_status *cs = list_entry (e, struct child_status, elem);
		child_status_release (cs); // child_status에 대한 부모의 참조 카운트를 1 줄이기
	}
}

void
process_exit_with_status (int status) {
	thread_current ()->exit_status = status;
	thread_exit ();
	NOT_REACHED ();
}

/* Starts the first userland program, called "initd", loaded from FILE_NAME.
 * The new thread may be scheduled (and may even exit)
 * before process_create_initd() returns. Returns the initd's
 * thread id, or TID_ERROR if the thread cannot be created.
 * Notice that THIS SHOULD BE CALLED ONCE. */
tid_t
process_create_initd (const char *file_name) {
	struct thread *parent = thread_current ();
	struct child_status *cs;
	struct initd_aux *aux;
	char *fn_copy;
	char *thread_name;
	char *space;
	tid_t tid;

	/* Make a copy of FILE_NAME.
	 * Otherwise there's a race between the caller and load(). */
	/* FILE_NAME의 복사본을 만드세요.
	 * 그렇지 않으면 호출자와 load() 함수 사이에 경쟁 조건이 발생합니다. */
	/* fn_copy는 initd가 process_exec()에 넘길 전체 command line이고,
	   thread_name은 스레드 이름으로 쓸 argv[0]만 따로 잘라낸 사본이다.
	   child_status는 initd도 부모가 wait할 수 있는 자식으로 관리하기 위해 만든다. */

	process_user_init (parent);

	// fn_copy 준비
	fn_copy = palloc_get_page (0);
	if (fn_copy == NULL)
		return TID_ERROR;
	strlcpy (fn_copy, file_name, PGSIZE);

	thread_name = palloc_get_page (0);
	if (thread_name == NULL) {
		palloc_free_page (fn_copy);
		return TID_ERROR;
	}
	strlcpy (thread_name, file_name, PGSIZE);

	// copied_file_name의 첫 번째 인자만 추출하기
	space = strchr (thread_name, ' ');
	if (space != NULL) {
		*space = '\0';
	}

	/* child_status 생성 및 parent->children에 등록 */

	// 자식 상태 구조체 할당
	cs = malloc (sizeof *cs);
	if (cs == NULL) {
		palloc_free_page (fn_copy);
		palloc_free_page (thread_name);
		return TID_ERROR;
	}

	/* cs 초기화 */
	cs->tid = TID_ERROR;
	cs->exit_status = -1;
	cs->waited = false;
	cs->load_done = false;
	cs->load_success = false;
	cs->ref_cnt = 2;
	sema_init (&cs->load_sema, 0);
	sema_init (&cs->exit_sema, 0);

	aux = malloc (sizeof *aux);
	if (aux == NULL) {
		free (cs);
		palloc_free_page (fn_copy);
		palloc_free_page (thread_name);
		return TID_ERROR;
	}

	// 전달하는 aux 안에 내용물 채우기
	aux->file_name = fn_copy;
	aux->child_info = cs; // aux에 child_status 연결

	// 현재 thread의 자식 목록에 넣음
	list_push_back (&parent->children, &cs->elem);

	/* Create a new thread to execute FILE_NAME. */
	tid = thread_create (thread_name, PRI_DEFAULT, initd, aux);
	palloc_free_page (thread_name); // thread_name 메모리 해제
	// (참고) aux 메모리는 initd() 안에서 file_name, child_info를 꺼낸 뒤 해제

	if (tid == TID_ERROR) {
		list_remove (&cs->elem);
		free (aux);
		free (cs);
		palloc_free_page (fn_copy);
		return TID_ERROR;
	}

	cs->tid = tid; // 자식 스레드 아이디에 새로 생성한 tid 할당

	return tid;
}

/* A thread function that launches first user process. */
static void
initd (void *aux) {
#ifdef VM
	supplemental_page_table_init (&thread_current ()->spt);
#endif

	struct initd_aux *_aux = aux;
	char *f_name = _aux->file_name;

	/* 현재 thread의 종료 상태를 부모에게 알릴 child_status를 연결한다. */
	thread_current ()->child_info = _aux->child_info;
	free (_aux);

	process_init ();

	if (process_exec (f_name) < 0)
		PANIC ("Fail to launch initd\n");
	NOT_REACHED ();
}

/* Clones the current process as `name`. Returns the new process's thread id, or
 * TID_ERROR if the thread cannot be created. */
tid_t
process_fork (const char *name, struct intr_frame *if_) {
	struct thread *parent = thread_current (); // 현재 스레드가 부모 스레드가 됨

	/* process_fork()에서 parent intr_frame을 child에게 넘길 수 있게 aux 구조체 할당.*/
	/* fork_aux는 child thread가 시작된 뒤 parent의 실행 문맥과 child_status를
	   이어받을 수 있게 잠시 사용하는 전달 객체다. */
	struct fork_aux *aux = malloc (sizeof *aux);
	if (aux == NULL) {
		return TID_ERROR;
	}

	// 자식 상태 구조체 할당
	struct child_status *cs = malloc (sizeof *cs);
	if (cs == NULL) {
		free (aux);
		return TID_ERROR;
	}

	/* cs 초기화 */
	cs->tid = TID_ERROR;
	cs->exit_status = -1;
	cs->waited = false;
	cs->load_done = false;
	cs->load_success = false;
	cs->ref_cnt = 2;
	sema_init (&cs->load_sema, 0);
	sema_init (&cs->exit_sema, 0);

	// 현재 실행 중인 부모 thread의 children 리스트에 새 자식 상태 cs를 등록
	list_push_back (&parent->children, &cs->elem);

	aux->child_info = cs; // child 저장

	aux->parent = parent; // parent 저장

	/* 부모의 syscall 진입 시점 레지스터 상태를 자식에게 넘기기 위해 aux에 복사한다. */
	memcpy (&aux->parent_if, if_, sizeof *if_); // parent intr_frame 복사

	/* thread_create에 parent thread 대신 aux 전달. __do_fork()에서는 aux를 받아 intr_frame 복사 */
	tid_t tid = thread_create (name, PRI_DEFAULT, __do_fork, aux);
	if (tid == TID_ERROR) {
		list_remove (&cs->elem);
		free (cs);
		free (aux);
		return TID_ERROR;
	}

	// 부모, 자식의 tid 동기화
	cs->tid = tid;
	/* parent는 child가 주소 공간과 fd table 복제를 끝내기 전까지 fork()를 반환하면 안 된다.
	   load_sema/load_success는 exec의 load 결과뿐 아니라 fork 준비 결과 동기화에도 사용한다. */
	sema_down (&cs->load_sema);
	if (!cs->load_success) {
		list_remove (&cs->elem);
		child_status_release (cs);
		return TID_ERROR;
	}

	return tid;
}

#ifndef VM
/* Duplicate the parent's address space by passing this function to the
 * pml4_for_each. This is only for the project 2. */
static bool
duplicate_pte (uint64_t *pte, void *va, void *aux) {
	struct thread *current = thread_current ();
	struct thread *parent = (struct thread *) aux;
	void *parent_page;
	void *newpage;
	bool writable;

	/* 1. If the parent_page is kernel page, then return immediately. */
	if (is_kern_pte (pte)) {
		return true;
	}

	/* 2. Resolve VA from the parent's page map level 4. */
	parent_page = pml4_get_page (parent->pml4, va);
	if (parent_page == NULL) {
		return false;
	}

	/* 3. Allocate new PAL_USER page for the child and set result to NEWPAGE. */
	newpage = palloc_get_page (PAL_USER);
	if (newpage == NULL) {
		return false;
	}

	/* 4. Duplicate parent's page to the new page and
	 *    check whether parent's page is writable or not (set WRITABLE
	 *    according to the result). */
	memcpy (newpage, parent_page, PGSIZE);

	/* 5. Add new page to child's page table at address VA with WRITABLE
	 *    permission. */
	writable = is_writable (pte);
	if (!pml4_set_page (current->pml4, va, newpage, writable)) {
		/* 6. if fail to insert page, do error handling. */
		palloc_free_page (newpage);
		return false;
	}

	return true;
}
#endif

/* A thread function that copies parent's execution context.
 * Hint) parent->tf does not hold the userland context of the process.
 *       That is, you are required to pass second argument of process_fork to
 *       this function. */

/* __do_fork()가 thread_current()만 받은 것이 아니라,
   process_fork()에서 만든 struct fork_aux를 받는 함수로 바뀌어야 함.
*/
static void
__do_fork (void *aux) {
	struct fork_aux *fork_aux = aux; // void *aux를 struct fork_aux *로 해석하기 위해 형변환
	struct intr_frame if_;           // 인터럽트/시스템콜 진입 시점의 CPU 레지스터 상태를 담는 구조체 변수
	struct thread *parent = fork_aux->parent;
	struct thread *current = thread_current ();
	struct intr_frame *parent_if = &fork_aux->parent_if;
	bool succ = true;

	current->child_info = fork_aux->child_info;

	/* 1. Read the cpu context to local stack. */
	/* fork()에서는 부모와 자식이 거의 같은 실행 상태에서 이어서 실행되어야 함.
	    그래서 부모의 intr_frame을 자식 쪽 로컬 변수 if_에 복사 */
	memcpy (&if_, parent_if, sizeof (struct intr_frame));
	/* child 쪽 fork() 반환값은 0이어야 하므로 복사한 intr_frame의 rax만 덮어쓴다. */
	if_.R.rax = 0; // 자식은 fork()의 반환값이 0이어야 함

	// 사용한 메모리 해제
	free (fork_aux);
	fork_aux = NULL;

	/* 2. Duplicate PT */
	/* 주소 공간 복제 */
	current->pml4 = pml4_create ();
	if (current->pml4 == NULL) {
		succ = false;
		goto done;
	}

	process_activate (current);
#ifdef VM
	supplemental_page_table_init (&current->spt);
	if (!supplemental_page_table_copy (&current->spt, &parent->spt)) {
		succ = false;
		goto done;
	}
#else
	if (!pml4_for_each (parent->pml4, duplicate_pte, parent)) {
		succ = false;
		goto done;
	}
#endif

	/* 자식 userprog 상태 초기화 */
	process_init ();

	/* 부모 fd table 복제 */
	if (!process_duplicate_fds (current, parent)) {
		succ = false;
		goto done;
	}

	/* 성공/실패 결과를 child_status에 기록. 부모에게 fork 준비 완료를 알림 */
done:
	/* 이 신호를 받은 parent가 process_fork()에서 child tid 또는 TID_ERROR를 반환한다. */
	current->child_info->load_success = succ;
	current->child_info->load_done = true;
	sema_up (&current->child_info->load_sema); // sema_up()으로 부모 깨움

	/* Finally, switch to the newly created process. */
	if (succ) {
		do_iret (&if_); // 성공이면 do_iret()
	}

	thread_exit (); // 실패면 thread_exit()
}

/* Switch the current execution context to the f_name.
 * Returns -1 on fail. */
int
process_exec (void *f_name) {
	struct thread *cur = thread_current ();
	char *file_name = f_name;
	bool success;

	/* We cannot use the intr_frame in the thread structure.
	 * This is because when current thread rescheduled,
	 * it stores the execution information to the member. */
	struct intr_frame _if;
	_if.ds = _if.es = _if.ss = SEL_UDSEG;
	_if.cs = SEL_UCSEG;
	_if.eflags = FLAG_IF | FLAG_MBS;

	/* 기존 실행 파일 정리 */
	process_close_exec_file (cur);

	/* 기존 사용자 주소 공간을 제거한 뒤 새 ELF 이미지를 같은 thread에 적재한다.
	   성공하면 do_iret()로 user mode에 진입하므로 이 함수는 호출자에게 돌아오지 않는다. */
	process_cleanup ();

	/* And then load the binary */
	success = load (file_name, &_if);

	/* If load failed, quit. */
	palloc_free_page (file_name);
	if (!success)
		return -1;

	/* Start switched process. */
	do_iret (&_if);
	NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
 * it was terminated by the kernel (i.e. killed due to an
 * exception), returns -1.  If TID is invalid or if it was not a
 * child of the calling process, or if process_wait() has already
 * been successfully called for the given TID, returns -1
 * immediately, without waiting.
 */
/* 스레드 TID가 종료될 때까지 기다렸다가 종료 상태를 반환합니다.
 * 커널에 의해 종료된 경우(예: 예외로 인해 강제 종료된 경우)
 * -1을 반환합니다. TID가 유효하지 않거나 호출 프로세스의 자식이 아니거나,
 * process_wait()가 이미 해당 TID에 대해 성공적으로 호출된 경우,
 * 대기하지 않고 즉시 -1을 반환합니다.
 */

int
process_wait (tid_t child_tid) {
	/* XXX: Hint) The pintos exit if process_wait (initd), we recommend you
	 * XXX:       to add infinite loop here before
	 * XXX:       implementing the process_wait. */

	struct child_status *cs;
	int status;

	/* 직접 자식이 아니면 기다릴 수 없다. */
	cs = process_find_child (child_tid);
	if (cs == NULL) {
		return -1;
	}

	// 이미 한 번 wait() 한 자식이면, 다시 기다릴 수 없으므로 실패 처리
	if (cs->waited) {
		return -1;
	}

	cs->waited = true;

	/*  자식이 아직 종료되지 않은 동안 부모는 여기서 멈춤.
	exit_sema는 부모와 자식이 공유하는 child_status 안에 존재. */
	/* 자식이 이미 종료된 경우 exit_sema가 올라가 있으므로 즉시 통과하고,
	   아직 실행 중이면 process_exit()에서 exit_sema를 올릴 때까지 대기한다. */
	sema_down (&cs->exit_sema); // 부모는 cs->exit_sema 값이 0이면 잠들고, 1이면 깨어남.

	status = cs->exit_status; // 자식의 종료상태를 cs 해제 전에 복사

	/* wait은 한 번만 성공할 수 있으므로 회수한 child_status를 children 목록에서 제거한다. */
	list_remove (&cs->elem); // 부모의 children 리스트에서 자식의 child_status를 제거
	child_status_release (cs);

	return status;
}

/* Exit the process. This function is called by thread_exit (). */
void
process_exit (void) {
	struct thread *curr = thread_current ();
	printf ("%s: exit(%d)\n", curr->name, curr->exit_status);
	process_close_all_files ();
	process_close_exec_file (curr);

	/* wait하지 않은 자식들의 parent-side child_status 참조 해제 */
	process_release_children (curr);

	// 부모가 wait()에서 받을 수 있도록 현재 프로세스의 종료 상태를 기록
	if (curr->child_info != NULL) {
		curr->child_info->exit_status = curr->exit_status;
		sema_up (&curr->child_info->exit_sema);
		child_status_release (curr->child_info); // 자식 종료 시 자식 쪽 참조를 내려야 함.
		curr->child_info = NULL;
	}

	process_cleanup ();
}

/* Free the current process's resources. */
static void
process_cleanup (void) {
	struct thread *curr = thread_current ();

#ifdef VM
	supplemental_page_table_kill (&curr->spt);
#endif

	uint64_t *pml4;
	/* Destroy the current process's page directory and switch back
	 * to the kernel-only page directory. */
	pml4 = curr->pml4;
	if (pml4 != NULL) {
		/* Correct ordering here is crucial.  We must set
		 * cur->pagedir to NULL before switching page directories,
		 * so that a timer interrupt can't switch back to the
		 * process page directory.  We must activate the base page
		 * directory before destroying the process's page
		 * directory, or our active page directory will be one
		 * that's been freed (and cleared). */
		curr->pml4 = NULL;
		pml4_activate (NULL);
		pml4_destroy (pml4);
	}
}

/* Sets up the CPU for running user code in the nest thread.
 * This function is called on every context switch. */
void
process_activate (struct thread *next) {
	/* Activate thread's page tables. */
	pml4_activate (next->pml4);

	/* Set thread's kernel stack for use in processing interrupts. */
	tss_update (next);
}

/* We load ELF binaries.  The following definitions are taken
 * from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
#define EI_NIDENT 16

#define PT_NULL    0          /* Ignore. */
#define PT_LOAD    1          /* Loadable segment. */
#define PT_DYNAMIC 2          /* Dynamic linking info. */
#define PT_INTERP  3          /* Name of dynamic loader. */
#define PT_NOTE    4          /* Auxiliary info. */
#define PT_SHLIB   5          /* Reserved. */
#define PT_PHDR    6          /* Program header table. */
#define PT_STACK   0x6474e551 /* Stack segment. */

#define PF_X 1 /* Executable. */
#define PF_W 2 /* Writable. */
#define PF_R 4 /* Readable. */

#define MAX_ARGV 64 /* 최대 인자 개수 */

/* Executable header.  See [ELF1] 1-4 to 1-8.
 * This appears at the very beginning of an ELF binary. */
struct ELF64_hdr {
	unsigned char e_ident[EI_NIDENT];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
};

struct ELF64_PHDR {
	uint32_t p_type;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr;
	uint64_t p_paddr;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;
};

/* Abbreviations */
#define ELF  ELF64_hdr
#define Phdr ELF64_PHDR

static bool setup_stack (struct intr_frame *if_);
static bool validate_segment (const struct Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
 * Stores the executable's entry point into *RIP
 * and its initial stack pointer into *RSP.
 * Returns true if successful, false otherwise. */
static bool
load (char *file_name, struct intr_frame *if_) {
	struct thread *t = thread_current ();
	struct ELF ehdr;
	struct file *file = NULL;
	off_t file_ofs;
	bool success = false;
	int i;
	size_t padding; // word-align

	/* Allocate and activate page directory. */
	/* 해당 프로세스의 사용자 가상 주소 공간 */
	t->pml4 = pml4_create ();
	if (t->pml4 == NULL) {
		goto done;
	}

	process_activate (thread_current ());

	// 토크나이저 사용
	int64_t argc = 0;         // argv 개수
	char *tmp_argv[MAX_ARGV]; // 임시 argv 배열
	char *argv[MAX_ARGV + 1]; // argv 배열
	char *save_ptr = NULL;    // file_name의 마지막 주소 보관
	char *token;

	for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)) {
		// 메모리 오염 방어 코드
		if (argc >= MAX_ARGV) {
			goto done;
		}

		tmp_argv[argc] = token;
		argc++;
	}

	// filesys_open(tmp_argv[0])에서 초기화되지 않은 값을 쓰는 상황을 막기 위한 방어 코드
	if (argc == 0) {
		goto done;
	}

	/* Open executable file. */
	file = filesys_open (tmp_argv[0]);
	if (file == NULL) {
		printf ("load: %s: open failed\n", tmp_argv[0]);
		goto done;
	}

	/* Read and verify executable header. */
	if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr || memcmp (ehdr.e_ident, "\177ELF\2\1\1", 7) || ehdr.e_type != 2 || ehdr.e_machine != 0x3E // amd64
	    || ehdr.e_version != 1 || ehdr.e_phentsize != sizeof (struct Phdr) || ehdr.e_phnum > 1024) {
		printf ("load: %s: error loading executable\n", file_name);
		goto done;
	}

	/* Read program headers. */
	file_ofs = ehdr.e_phoff;
	for (i = 0; i < ehdr.e_phnum; i++) {
		struct Phdr phdr;

		if (file_ofs < 0 || file_ofs > file_length (file))
			goto done;
		file_seek (file, file_ofs);

		if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
			goto done;
		file_ofs += sizeof phdr;
		switch (phdr.p_type) {
		case PT_NULL:
		case PT_NOTE:
		case PT_PHDR:
		case PT_STACK:
		default:
			/* Ignore this segment. */
			break;
		case PT_DYNAMIC:
		case PT_INTERP:
		case PT_SHLIB:
			goto done;
		case PT_LOAD:
			if (validate_segment (&phdr, file)) {
				bool writable = (phdr.p_flags & PF_W) != 0;
				uint64_t file_page = phdr.p_offset & ~PGMASK;
				uint64_t mem_page = phdr.p_vaddr & ~PGMASK;
				uint64_t page_offset = phdr.p_vaddr & PGMASK;
				uint32_t read_bytes, zero_bytes;
				if (phdr.p_filesz > 0) {
					/* Normal segment.
					 * Read initial part from disk and zero the rest. */
					read_bytes = page_offset + phdr.p_filesz;
					zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE) - read_bytes);
				} else {
					/* Entirely zero.
					 * Don't read anything from disk. */
					read_bytes = 0;
					zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
				}
				if (!load_segment (file, file_page, (void *) mem_page,
				                   read_bytes, zero_bytes, writable))
					goto done;
			} else
				goto done;
			break;
		}
	}

	/* Set up stack. */
	if (!setup_stack (if_)) {
		goto done;
	}

	/* Start address. */
	if_->rip = ehdr.e_entry;

	// 각 문자열의 주소를 스택에 오른쪽에서 왼쪽 순서로 push합니다.(tmp_argv 끝부터 역순으로 넣는다.)
	if_->rsp = USER_STACK;

	// 문자열들을 복사하여 실제 argv 배열에 역순으로 넣기
	for (int i = argc - 1; i >= 0; i--) {
		size_t len = strlen (tmp_argv[i]) + 1;

		if_->rsp -= len;                              // 넣을 만큼 빼주어야 함
		memcpy ((void *) if_->rsp, tmp_argv[i], len); // start / end / sizeof

		// argv[] 에 스택의 주소값 저장
		argv[i] = (char *) if_->rsp;
		// printf("if_->rsp === %x\n", if_->rsp); // debug
	}

	// padding으로 rsp를 8바이트 정렬
	padding = if_->rsp % 8; // 8의 배수로 맞추기 위한 패딩
	if_->rsp -= padding;    // 스택 넘버가 감소하는 방향으로 진행되기 때문

	// NULL sentinel: argv[argc]가 null 포인터가 되도록 넣기
	argv[argc] = NULL;

	// 위에서 넣은 argv의 주소값을 8바이트 단위로 스택에 넣기.
	for (int i = argc; i >= 0; i--) {
		// 넣을 만큼 빼주어야 함
		if_->rsp -= sizeof (argv[i]);

		// 스택에 argv[]의 인자의 주소값 저장
		memcpy ((void *) if_->rsp, &argv[i], sizeof (argv[i]));
		// printf("&argv[i] === %x\n", argv[i]); // debug
	}

	// %rsi가 argv(argv[0]의 주소)를 가리키게 하고, %rdi에는 argc를 넣습니다.
	if_->R.rsi = if_->rsp;
	if_->R.rdi = argc;

	// fake address 반환 - 문자열 "0"이 아닌 8바이트짜리 NULL 값을 넣어야 함.
	void *fake_ret = NULL;
	if_->rsp -= sizeof (fake_ret);
	memcpy ((void *) if_->rsp, &fake_ret, sizeof (fake_ret));

	success = true;

	// rox 구현: 파일 로드가 성공한 경우에는 닫지 않고 현재 thread에 보관
	/* 실행 중인 파일에 대한 쓰기를 막기 위해 deny_write를 걸고 thread에 보관한다.
	   이 file은 process_exec()로 다른 프로그램을 실행하거나 process_exit()할 때 닫힌다. */
	file_deny_write (file);
	thread_current ()->exec_file = file;

done:
	/* We arrive here whether the load is successful or not. */
	/* load 실패 시에는 실행 파일을 thread에 보관하지 않았으므로 여기서 닫아 누수를 막는다. */
	if (!success && file != NULL) {
		file_close (file);
	}

	return success;
}

/* Checks whether PHDR describes a valid, loadable segment in
 * FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Phdr *phdr, struct file *file) {
	/* p_offset and p_vaddr must have the same page offset. */
	if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK))
		return false;

	/* p_offset must point within FILE. */
	if (phdr->p_offset > (uint64_t) file_length (file))
		return false;

	/* p_memsz must be at least as big as p_filesz. */
	if (phdr->p_memsz < phdr->p_filesz)
		return false;

	/* The segment must not be empty. */
	if (phdr->p_memsz == 0)
		return false;

	/* The virtual memory region must both start and end within the
	   user address space range. */
	if (!is_user_vaddr ((void *) phdr->p_vaddr))
		return false;
	if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
		return false;

	/* The region cannot "wrap around" across the kernel virtual
	   address space. */
	if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
		return false;

	/* Disallow mapping page 0.
	   Not only is it a bad idea to map page 0, but if we allowed
	   it then user code that passed a null pointer to system calls
	   could quite likely panic the kernel by way of null pointer
	   assertions in memcpy(), etc. */
	if (phdr->p_vaddr < PGSIZE)
		return false;

	/* It's okay. */
	return true;
}

#ifndef VM
/* Codes of this block will be ONLY USED DURING project 2.
 * If you want to implement the function for whole project 2, implement it
 * outside of #ifndef macro. */

/* load() helpers. */
static bool install_page (void *upage, void *kpage, bool writable);

/* Loads a segment starting at offset OFS in FILE at address
 * UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
 * memory are initialized, as follows:
 *
 * - READ_BYTES bytes at UPAGE must be read from FILE
 * starting at offset OFS.
 *
 * - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.
 *
 * The pages initialized by this function must be writable by the
 * user process if WRITABLE is true, read-only otherwise.
 *
 * Return true if successful, false if a memory allocation error
 * or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) {
	ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
	ASSERT (pg_ofs (upage) == 0);
	ASSERT (ofs % PGSIZE == 0);

	file_seek (file, ofs);
	while (read_bytes > 0 || zero_bytes > 0) {
		/* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* Get a page of memory. */
		uint8_t *kpage = palloc_get_page (PAL_USER);
		if (kpage == NULL)
			return false;

		/* Load this page. */
		if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes) {
			palloc_free_page (kpage);
			return false;
		}
		memset (kpage + page_read_bytes, 0, page_zero_bytes);

		/* Add the page to the process's address space. */
		if (!install_page (upage, kpage, writable)) {
			printf ("fail\n");
			palloc_free_page (kpage);
			return false;
		}

		/* Advance. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		upage += PGSIZE;
	}
	return true;
}

/* Create a minimal stack by mapping a zeroed page at the USER_STACK */
static bool
setup_stack (struct intr_frame *if_) {
	uint8_t *kpage;
	bool success = false;

	kpage = palloc_get_page (PAL_USER | PAL_ZERO);
	if (kpage != NULL) {
		success = install_page (((uint8_t *) USER_STACK) - PGSIZE, kpage, true);
		if (success)
			if_->rsp = USER_STACK;
		else
			palloc_free_page (kpage);
	}
	return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
 * virtual address KPAGE to the page table.
 * If WRITABLE is true, the user process may modify the page;
 * otherwise, it is read-only.
 * UPAGE must not already be mapped.
 * KPAGE should probably be a page obtained from the user pool
 * with palloc_get_page().
 * Returns true on success, false if UPAGE is already mapped or
 * if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable) {
	struct thread *t = thread_current ();

	/* Verify that there's not already a page at that virtual
	 * address, then map our page there. */
	return (pml4_get_page (t->pml4, upage) == NULL && pml4_set_page (t->pml4, upage, kpage, writable));
}
#else
/* From here, codes will be used after project 3.
 * If you want to implement the function for only project 2, implement it on the
 * upper block. */

static bool
lazy_load_segment (struct page *page, void *aux) {
	/* TODO: Load the segment from the file */
	/* TODO: This called when the first page fault occurs on address VA. */
	/* TODO: VA is available when calling this function. */
}

/* Loads a segment starting at offset OFS in FILE at address
 * UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
 * memory are initialized, as follows:
 *
 * - READ_BYTES bytes at UPAGE must be read from FILE
 * starting at offset OFS.
 *
 * - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.
 *
 * The pages initialized by this function must be writable by the
 * user process if WRITABLE is true, read-only otherwise.
 *
 * Return true if successful, false if a memory allocation error
 * or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) {
	ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
	ASSERT (pg_ofs (upage) == 0);
	ASSERT (ofs % PGSIZE == 0);

	while (read_bytes > 0 || zero_bytes > 0) {
		/* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* TODO: Set up aux to pass information to the lazy_load_segment. */
		void *aux = NULL;
		if (!vm_alloc_page_with_initializer (VM_ANON, upage,
		                                     writable, lazy_load_segment, aux))
			return false;

		/* Advance. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		upage += PGSIZE;
	}
	return true;
}

/* Create a PAGE of stack at the USER_STACK. Return true on success. */
static bool
setup_stack (struct intr_frame *if_) {
	bool success = false;
	void *stack_bottom = (void *) (((uint8_t *) USER_STACK) - PGSIZE);

	/* TODO: Map the stack on stack_bottom and claim the page immediately.
	 * TODO: If success, set the rsp accordingly.
	 * TODO: You should mark the page is stack. */
	/* TODO: Your code goes here */

	return success;
}
#endif /* VM */
