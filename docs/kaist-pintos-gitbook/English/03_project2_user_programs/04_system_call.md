# 04_system_call

### System Calls

**Implement system call infrastructure.**

Implement the system call handler in `userprog/syscall.c`. The skeleton
implementation we provide "handles" system calls by terminating the process. It
will need to retrieve the system call number, then any system call arguments,
and carry out appropriate actions.

#### System Call Details

The first project already dealt with one way that the operating system can
regain control from a user program: interrupts from timers and I/O devices.
These are "external" interrupts, because they are caused by entities outside
the CPU.

The operating system also deals with software exceptions, which are events that
occur in program code. These can be errors such as a page fault or division by
zero. Exceptions are also the means by which a user program can request services
("system calls") from the operating system.

In traditional x86 architecture, the system call was handled as same as other
software exceptions. However, in x86-64, manufacturers introduce special
instruction for the system calls, `syscall`. This provides fast way to call the
system call handler.

Nowaday, the `syscall` instruction is the most commonly used means for invoking
system calls in x86-64. In Pintos, user programs invoke `syscall` to make a
system call. The system call number and any additional arguments are expected to
be set to the registers in the normal fashion before invoking the `syscall`
instruction except two points:

- `%rax` is the system call number.
- The fourth argument is `%r10`, not `%rcx`.

Thus, when the system call handler `syscall_handler()` gets control, the system
call number is in the rax, and arguments are passed with the order %rdi, %rsi,
%rdx, %r10, %r8, and %r9.

The caller's registers are accessible to `struct intr_frame` passed to it.
(`struct intr_frame` is on the kernel stack.)

The x86-64 convention for function return values is to place them in the RAX
register. System calls that return a value can do so by modifying the `rax`
member of `struct intr_frame`.

#### Implement the following system calls.

The prototypes listed are those seen by a user program that includes
`include/lib/user/syscall.h` (This header, and all others in `include/lib/user`,
are for use by user programs only). System call numbers for each system call are
defined in `include/lib/syscall-nr.h`:

---

```c
void halt (void);
```

> Terminates Pintos by calling `power_off()` (declared in
> `src/include/threads/init.h`). This should be seldom used, because you lose
> some information about possible deadlock situations, etc.

---

```c
void exit (int status);
```

> Terminates the current user program, returning `status` to the kernel. If the
> process's parent `wait`s for it (see below), this is the status that will be
> returned. Conventionally, a `status` of `0` indicates success and nonzero
> values indicate errors.

---

```c
pid_t fork (const char *thread_name);
```

> Create new process which is the clone of current process with the name
> THREAD_NAME. You don't need to clone the value of the registers except `%RBX`,
> `%RSP`, `%RBP`, and `%R12` - `%R15`, which are callee-saved registers.
> Must return pid of the child process, otherwise shouldn't be a valid pid. In
> child process, the return value should be `0`. The child
> should have **DUPLICATED** resources including file descriptor and virtual
> memory space. Parent process should never return from the fork until it knows
> whether the child process successfully cloned. That is, if the child process
> fail to duplicate the resource, the fork () call of parent should return the
> TID_ERROR.
>
> The template utilizes the `pml4_for_each()` in `threads/mmu.c` to
> copy entire user memory space, including corresponding pagetable structures,
> but you need to fill missing parts of passed `pte_for_each_func`
> (See [virtual address](../06_appendix/04_virtual_address.md)).

---

```c
int exec (const char *cmd_line);
```

> Change current process to the executable whose name is given in `cmd_line`,
> passing any given arguments. This never returns if successful. Otherwise the
> process *terminates* with exit state `-1`, if the program cannot load or run
> for any reason. This function does not change the name of the thread that
> called `exec`. Please note that file descriptors remain open across an `exec`
> call.

---

```c
int wait (pid_t pid);
```

> Waits for a child process `pid` and retrieves the child's exit status. If
> `pid` is still alive, waits until it terminates. Then, returns the status that
> `pid` passed to exit. If `pid` did not call `exit()`, but was terminated by the
> kernel (e.g. killed due to an exception), `wait(pid)` must return `-1`. It is
> perfectly legal for a parent process to wait for child processes that have
> already terminated by the time the parent calls wait, but the kernel must
> still allow the parent to retrieve its child’s exit status, or learn that the
> child was terminated by the kernel.
>
> `wait` must fail and return `-1` immediately if any of the following
> conditions is true:
>
> - `pid` does not refer to a direct child of the calling process. `pid` is a
>   direct child of the calling process if and only if the calling process
>   received `pid` as a return value from a successful call to `fork`.
>   Note that children are not inherited: if A spawns child B and B spawns child
>   process C, then A cannot wait for C, even if B is dead. A call to `wait(C)` by
>   process A must fail. Similarly, orphaned processes are not assigned to a new
>   parent if their parent process exits before they do.
> - The process that calls `wait` has already called `wait` on `pid`. That is, a
>   process may `wait` for any given child at most once.

Processes may spawn any number of children, `wait` for them in any order, and
may even exit without having waited for some or all of their children. Your
design should consider all the ways in which waits can occur. All of a
process's resources, including its `struct thread`, must be freed whether its
parent ever waits for it or not, and regardless of whether the child exits
before or after its parent.

**You must ensure that Pintos does not terminate until the initial process exits**.
The supplied Pintos code tries to do this by calling `process_wait()` (in
`userprog/process.c`) from `main()` (in `threads/init.c`). We suggest that you
implement `process_wait()` according to the comment at the top of the function
and then implement the wait system call in terms of `process_wait()`.

Implementing this system call requires considerably more work than any of the
rest.

---

```c
bool create (const char *file, unsigned initial_size);
```

> Creates a new file called `file` initially `initial_size` bytes in size.
> Returns true if successful, false otherwise. Creating a new file does not open
> it: opening the new file is a separate operation which would require a `open`
> system call.

---

```c
bool remove (const char *file);
```

> Deletes the file called `file`. Returns true if successful, false otherwise. A
> file may be removed regardless of whether it is open or closed, and removing
> an open file does not close it. See Removing an Open File in [FAQ](08_faq.md)
> for details.

---

```c
int open (const char *file);
```

> Opens the file called `file`. Returns a nonnegative integer handle called a
> "file descriptor" (fd), or `-1` if the file could not be opened.
> File descriptors numbered 0 and 1 are reserved for the console: fd 0
> (`STDIN_FILENO`) is standard input, fd 1 (`STDOUT_FILENO`) is standard output.
> The `open` system call will never return either of these file descriptors,
> which are valid as system call arguments only as explicitly described below.
> Each process has an independent set of file descriptors. File descriptors are
> inherited by child processes. When a single file is opened more than once,
> whether by a single process or different processes, each open returns a new
> file descriptor. Different file descriptors for a single file are closed
> independently in separate calls to close and they do not share a file
> position. **You should follow the linux scheme, which returns integer starting**​**from zero, to do the extra.**

---

```c
int filesize (int fd);
```

> Returns the size, in bytes, of the file open as `fd`.

---

```c
int read (int fd, void *buffer, unsigned size);
```

> Reads `size` bytes from the file open as `fd` into `buffer`. Returns the
> number of bytes actually read (`0` at end of file), or `-1` if the file could
> not be read (due to a condition other than end of file). `fd` 0 reads from the
> keyboard using `input_getc()`.

---

```c
int write (int fd, const void *buffer, unsigned size);
```

> Writes `size` bytes from `buffer` to the open file `fd`. Returns the number of
> bytes actually written, which may be less than `size` if some bytes could not
> be written. Writing past end-of-file would normally extend the file, but file
> growth is not implemented by the basic file system. The expected behavior is
> to write as many bytes as possible up to end-of-file and return the actual
> number written, or `0` if no bytes could be written at all. `fd` 1 writes to
> the console. Your code to write to the console should write all of buffer in
> one call to `putbuf()`, at least as long as size is not bigger than a few
> hundred bytes (It is reasonable to break up larger buffers). Otherwise, lines
> of text output by different processes may end up interleaved on the console,
> confusing both human readers and our grading scripts.

---

```c
void seek (int fd, unsigned position);
```

> Changes the next byte to be read or written in open file `fd` to `position`,
> expressed in bytes from the beginning of the file (Thus, a `position` of `0`
> is the file's start). A seek past the current end of a file is not an error. A
> later `read` obtains 0 bytes, indicating end of file. A later `write` extends
> the file, filling any unwritten gap with zeros. (However, in Pintos files have
> a fixed length until project 4 is complete, so `write`s past end of file will
> return an error.) These semantics are implemented in the file system and do
> not require any special effort in system call implementation.

---

```c
unsigned tell (int fd);
```

> Returns the position of the next byte to be read or written in open file `fd`,
> expressed in bytes from the beginning of the file.

---

```c
void close (int fd);
```

> Closes file descriptor `fd`. Exiting or terminating a process implicitly
> closes all its open file descriptors, as if by calling this function for each
> one.

The file defines other syscalls. Ignore them for now. You will implement some of
them in project 3 and the rest in project 4, so be sure to design your system
with extensibility in mind.

You must synchronize system calls so that any number of user processes can make
them at once. In particular, it is not safe to call into the file system code
provided in the `filesys` directory from multiple threads at once. Your system
call implementation must treat the file system code as a critical section.
Don't forget that `process_exec()` also accesses files. For now, we recommend
against modifying code in the `filesys` directory.

We have provided you a user-level function for each system call in
`lib/user/syscall.c`. These provide a way for user processes to invoke each
system call from a C program. Each uses a little inline assembly code to invoke
the system call and (if appropriate) returns the system call's return value.

When you're done with this part, and forevermore, Pintos should be bulletproof.
Nothing that a user program can do should ever cause the OS to crash, panic,
fail an assertion, or otherwise malfunction. It is important to emphasize this
point: our tests will try to break your system calls in many, many ways. You
need to think of all the corner cases and handle them. The sole way a user
program should be able to cause the OS to halt is by invoking the halt system
call.

If a system call is passed an invalid argument, acceptable options include
returning an error value (for those calls that return a value), returning an
undefined value, or terminating the process.
