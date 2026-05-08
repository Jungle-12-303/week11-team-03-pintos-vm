#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include <stdbool.h>
#include "threads/thread.h"
#include "threads/synch.h"

struct file;

struct child_status {
	tid_t tid;                  // 자식 thread의 id
	int exit_status;            // 자식의 종료 상태 - 자식이 exit(status) 하거나 예외로 죽으면 이 값이 기록되고 부모의 wait()가 이 값을 반환
	bool waited;                // 부모가 이미 이 자식에 대해 wait()를 호출했는지 표시
	bool load_done;             // 자식의 load 또는 fork 준비가 끝났는지 나타내는 상태값
	bool load_success;          // 자식이 실행 준비에 성공했는지 표시 - 실패하면 부모의 fork 반환값은 TID_ERROR 또는 -1
	int ref_cnt;                // child_status를 몇 쪽에서 아직 사용 중인지 세는 값. 처음에는 부모와 자식이 참조하므로 2
	struct semaphore load_sema; // 부모가 자식의 "생성/로드 준비 완료"를 기다릴 때 쓰는 세마포어
	struct semaphore exit_sema; // 부모가 자식 종료를 기다릴 때 쓰는 세마포어
	struct list_elem elem;      // 부모의 children 리스트에 child_status를 넣기 위한 리스트 연결 필드
};

tid_t process_create_initd (const char *file_name);
tid_t process_fork (const char *name, struct intr_frame *if_);
int process_exec (void *f_name);
int process_wait (tid_t);
void process_exit (void);
void process_user_init (struct thread *t);
void process_exit_with_status (int status) NO_RETURN;
void process_activate (struct thread *next);

int process_add_file (struct file *file);
struct file *process_get_file (int fd);
bool process_close_file (int fd);
void process_close_all_files (void);
bool process_duplicate_fds (struct thread *dst, struct thread *src);
struct child_status *process_find_child (tid_t tid);
void child_status_release (struct child_status *cs);

#endif /* userprog/process.h */
