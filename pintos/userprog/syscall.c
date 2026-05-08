#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "threads/mmu.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "userprog/gdt.h"
#include "userprog/process.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);
static bool user_addr_mapped (const void *uaddr);

struct lock filesys_lock;

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR         0xc0000081 /* Segment selector msr */
#define MSR_LSTAR        0xc0000082 /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	lock_init (&filesys_lock);

	write_msr (MSR_STAR, ((uint64_t) SEL_UCSEG - 0x10) << 48 |
	                             ((uint64_t) SEL_KCSEG) << 32);
	write_msr (MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr (MSR_SYSCALL_MASK,
	           FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* 주소 하나가 유효한 user virtual address인지 검사 */
static bool
user_addr_mapped (const void *uaddr) {
	struct thread *cur = thread_current ();

	return uaddr != NULL            // 주소가 비어있지 않고
	       && is_user_vaddr (uaddr) // 사용자 영역이면서
	       && cur->pml4 != NULL     // 현재 프로세스의 페이지 테이블이 존재하고 (pml4: 사용자 가상 주소를 실제 물리/커널 주소로 변환할 때 쓰는 최상위 페이지 테이블)
	       // &&은 short-circuit이므로, 순차적으로 조건을 검사하다가 위 조건을 만족하지 못하면 아래 조건은 검사하지 않음
	       && pml4_get_page (cur->pml4, uaddr) != NULL; // 페이지 테이블에 실제로 매핑된 주소이면 True
}

/* 주소가 유효하지 않으면 현재 process를 exit(-1)하는 공개 검증 함수 */
void
user_check_ptr (const void *uaddr) {
	if (!user_addr_mapped (uaddr))
		process_exit_with_status (-1);
}

/* 이하는 syscall 종류에 맞게 user_check_ptr()를 반복/조합해서 쓰는 상위 helper 함수*/

/* user buffer를 커널이 읽을 때 검증
   write(fd, buffer, size)에서 buffer 검증 */
void
user_check_read (const void *uaddr, size_t size) {
	uint64_t start;
	uint64_t last;
	uint64_t page;

	if (size == 0)
		return;
	if (uaddr == NULL)
		process_exit_with_status (-1);

	start = (uint64_t) uaddr;
	last = start + size - 1;

	if (last < start) // 오버플로우 검사
		process_exit_with_status (-1);

	for (page = (uint64_t) pg_round_down ((void *) start); // 버퍼의 시작 주소가 속한 페이지의 시작 위치
	     page <= (uint64_t) pg_round_down ((void *) last); // 버퍼의 마지막 주소가 속한 페이지의 시작 위치
	     page += PGSIZE)                                   // 페이지 사이즈만큼 증가
		user_check_ptr ((const void *) page);              // pml4_get_page()를 통해 page 주소가 현재 스레드의 pml4에서 찾을 수 있는지 확인, 찾을 수 없다면 프로세스 종료 (-1)
}

/* 커널이 user buffer에 써야 할 때 검증
   read(fd, buffer, size)에서 buffer 검증 */
void

user_check_write (void *uaddr, size_t size) {
	user_check_read (uaddr, size);
}

/* user가 넘긴 C 문자열 검증
   open, create, remove, exec, fork 이름 등에 사용 */
void
user_check_string (const char *uaddr) {
	const char *p;

	for (p = uaddr;; p++) { // 문자열 끝까지 검사
		user_check_ptr (p); // 특정 문자의 주소가 오류가 있는 경우 프로세스 종료
		if (*p == '\0')
			return;
	}
}

/* user 문자열을 kernel page로 복사
   exec처럼 이후 주소 공간이 바뀌거나 오래 보관해야 하는 경우에 사용 */
char *
user_strdup (const char *uaddr) {
	char *copy;
	size_t i;

	copy = palloc_get_page (0);
	if (copy == NULL)
		process_exit_with_status (-1);

	for (i = 0; i < PGSIZE; i++) { // 안전하게 한 글자씩 검사하면서 복사
		user_check_ptr (uaddr + i);
		copy[i] = uaddr[i];
		if (copy[i] == '\0')
			return copy;
	}

	// copy를 반환하지 않고 종료됐다면 오류가 있는 것이므로, 할당받은 페이지 반환 후 오류 종료
	palloc_free_page (copy);
	process_exit_with_status (-1);
}

/* 커널이 syscall 요청을 받아 실제로 처리하는 dispatcher */
void
syscall_handler (struct intr_frame *f) {
	// x86-64 호출 규약에서 함수 반환값은 RAX 레지스터에 두어야 합니다. 반환값이 있는 시스템 콜은 struct intr_frame의 rax 멤버를 수정해 이를 구현할 수 있습니다.

	switch (f->R.rax) {
	/* Projects 2 and later. */
	case SYS_HALT:    /* Halt the operating system. */
		power_off (); // OS 종료
		break;
	case SYS_EXIT: /* Terminate this process. */
		process_exit_with_status ((int) f->R.rdi);
		break;
	// 현재 실행중인 사용자 process를 복사해서 자식 process 생성
	case SYS_FORK: /* Clone current process. */
	{
		const char *thread_name = (const char *) f->R.rdi; // fork(const char *thread_name)의 첫 번째 인자

		user_check_string (thread_name);
		f->R.rax = process_fork (thread_name, f); // fork() syscall 실패시 부모에게 -1 반환, 성공시 child tid 반환
		break;
	}
	// 이미 실행 중인 user process가 exec()를 요청한 상황
	case SYS_EXEC: /* Switch current process. */
	{
		const char *cmd_line = (const char *) f->R.rdi; // exec(const char *file)의 첫 번째 인자
		char *cmd_copy;

		user_check_string (cmd_line);      // 사용자 입력값 검증
		cmd_copy = user_strdup (cmd_line); // user 문자열을 커널 페이지로 복사

		if (process_exec (cmd_copy) < 0) {
			process_exit_with_status (-1);
		}

		NOT_REACHED ();

		break;
	}
	case SYS_WAIT: /* Wait for a child process to die. */
	{
		tid_t child_tid = (tid_t) f->R.rdi;  // 사용자 프로그램이 넘긴 pid 값
		f->R.rax = process_wait (child_tid); // 자식 종료 상태를 syscall 반환값으로 저장
		break;
	}
	case SYS_CREATE: /* Create a file. */
	{
		// 값 들어 오는 것 확인
		// rdi로 제목 데이터 / rsi로 길이 데이터 들어옴.
		// file 먼저 찾아보고 있으면 에러 반환
		char *file = (char *) f->R.rdi;

		user_check_string (file);
		f->R.rax = filesys_create ((char *) f->R.rdi, f->R.rsi);
		break;
	}
	case SYS_REMOVE: /* Delete a file. */
	{
		user_check_ptr ((void *) f->R.rdi); // 제거하려는 파일의 포인터가 올바르지 않은 경우 오류
		// TODO
		break;
	}
	case SYS_OPEN: {
		// f->R.rax = filesys_open((char *)f->R.rdi);
		const char *file_name = (const char *) f->R.rdi;
		struct file *file;

		user_check_string (file_name);
		file = filesys_open (file_name);
		f->R.rax = process_add_file (file);
		break;
	}
	case SYS_FILESIZE: /* Obtain a file's size. */
	{
		int fd = (int) f->R.rdi;

		f->R.rax = file_length (process_get_file (fd));
		break;
	}
	case SYS_READ: // TODO                   /* Read from a file. */
	{
		int fd = (int) f->R.rdi;
		void *buffer = (void *) f->R.rsi;
		unsigned size = (unsigned) f->R.rdx;
		struct file *file;

		user_check_write (buffer, size);

		// fd 0일 때 키보드 입력값으로 대체.
		if (fd == 0) {
			char *buf = buffer;

			for (int i = 0; i < size; i++)
				buf[i] = input_getc ();

			f->R.rax = size;
		} // 파일을 특정했을 때
		else if (fd >= 2) {
			file = process_get_file (fd);

			if (file == NULL) {
				process_exit_with_status (-1);
			} else {
				// file_read로 읽어서 값 반환
				f->R.rax = file_read (file, buffer, size);
			}
		} else {
			process_exit_with_status (-1);
		}
		break;
	}
	case SYS_WRITE: {
		int fd = (int) f->R.rdi;
		void *buffer = (void *) f->R.rsi;
		off_t size = f->R.rdx;
		struct file *file;

		user_check_read (buffer, size);

		if (fd == 1) {
			putbuf (buffer, size);
			f->R.rax = size;
		} else if (fd >= 2) {
			// 열어둔 파일에 값을 입력한다는 의미니까
			file = process_get_file (fd);
			if (file == NULL) {
				f->R.rax = -1;
				break;
			}
			f->R.rax = file_write (file, buffer, size);
			// file_write(struct file * file, const void *buffer, off_t size)
		} else {
			f->R.rax = -1;
		}
		break;
	}
	case SYS_SEEK: /* Change position in a file. */
	{              /* 파일 안에서 현재 읽기/쓰기 위치를 바꾼다 */
		int fd = (int) f->R.rdi;
		unsigned position = (unsigned) f->R.rsi;
		struct file *file = process_get_file (fd); // 사용자 프로그램의 fd를 커널 내부의 struct file *로 변경

		if (file != NULL) {
			file_seek (file, position); // 열린 파일의 현재 위치 pos를 position으로 바꾸기
		}

		break;
	}
	case SYS_TELL: // TODO               /* Report current position in a file. */
		f->R.rax = -1;
		break;
	case SYS_CLOSE: // TODO                 /* Close a file. */
		process_close_file (f->R.rdi);
		break;
	default:
		process_exit_with_status (-1); // 프로세스 비정상 종료
		break;
	}
}
