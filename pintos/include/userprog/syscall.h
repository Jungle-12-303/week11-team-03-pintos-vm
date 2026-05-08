#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stddef.h>
#include "threads/synch.h"

extern struct lock filesys_lock;

void syscall_init (void);
void user_check_ptr (const void *uaddr);
void user_check_read (const void *uaddr, size_t size);
void user_check_write (void *uaddr, size_t size);
void user_check_string (const char *uaddr);
char *user_strdup (const char *uaddr);

#endif /* userprog/syscall.h */
