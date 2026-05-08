# 08_faq

# FAQ

### How much code will I need to write?

Here's a summary of our reference solution, produced by the `git diff --stat`.
The final row gives total lines inserted and deleted; a changed line counts as
both an insertion and a deletion.
The reference solution represents just one possible solution. Many other
solutions are also possible and many of those differ greatly from the reference
solution. Some excellent solutions may not modify all the files modified by the
reference solution, and some may modify files not modified by the reference
solution. Also, this includes implementation of extra requirement. FYI, ~150
lines are related to the extra.

```
src/include/threads/thread.h   |  23 ++
src/include/userprog/syscall.h |   3 +
src/threads/thread.c           |   5 +
src/userprog/exception.c       |   4 +
src/userprog/process.c         | 355 +++++++++++++++++++++++++++++++++++++++++-
src/userprog/syscall.c         | 429 ++++++++++++++++++++++++++++++++++++++++++-
6 files changed, 782 insertions(+), 37 deletions(-)
```

### The kernel always panics when I run `pintos -p file -- -q`.

Did you format the file system (with `pintos -f`)?

Is your file name too long? The file system limits file names to 14 characters.

A command like `pintos -p ../../examples/echo -- -q` will exceed the limit.

Use `pintos -p ../../examples/echo:echo -- -q` to put the file under the
name `echo` instead.

Is the file system full?

Does the file system already contain 16 files? The base Pintos file system has
a 16-file limit.

The file system may be so fragmented that there's not enough contiguous space
for your file.

### When I run `pintos -p ../file --`, 'file' isn't copied.

Files are written under the name you refer to them, by default, so in this case
the file copied in would be named '../file'. You probably want to run `pintos -p ../file:file --` instead.

### All my user programs die with page faults.

This will happen if you haven't implemented argument passing (or haven't done
so correctly). The basic C library for user programs tries to read argv off the
stack. If the stack or registers aren't properly set up, this causes a
page fault.

### All my user programs die with `system call!`

You'll have to implement system calls before you see anything else. Every
reasonable program tries to make at least one system call (`exit()`) and most
programs make more than that. Notably, `printf()` invokes the write system
call. The default system call handler just prints `system call!` and terminates
the program. Until then, you can use `hex_dump()` to convince yourself that
argument passing is implemented correctly.

### How can I disassemble user programs?

The objdump utility can disassemble entire user programs or object files. Invoke
it as objdump -d file. You can use GDB's disassemble command to disassemble
individual functions.

### Why do many C include files not work in Pintos programs?

### Can I use libfoo in my Pintos programs?

The C library we provide is very limited. It does not include many of the
features that are expected of a real operating system's C library. The C library
must be built specifically for the operating system (and architecture), since it
must make system calls for I/O and memory allocation. (Not all functions do,
of course, but usually the library is compiled as a unit.)

The chances are good that the library you want uses parts of the C library that
Pintos doesn't implement. It will probably take at least some porting effort to
make it work under Pintos. Notably, the Pintos user program C library does
not have a `malloc()` implementation.

### How do I compile new user programs?

Modify `src/examples/Makefile`, then run `make`.

### Can I run user programs under a debugger?

Yes, with some limitations. See [GDB](../06_appendix/06_debugging_tools.md#gdb).

### What's the difference between tid_t and pid_t?

A tid_t identifies a kernel thread, which may have a user process running
in it (if created with `process_fork()`) or not (if created with
`thread_create()`). It is a data type used only in the kernel.
A pid_t identifies a user process. It is used by user processes and the kernel
in the exec and wait system calls. You can choose whatever suitable types you
like for tid_t and pid_t. By default, they're both int. You can make them a
one-to-one mapping, so that the same values in both identify the same process,
or you can use a more complex mapping. It's up to you.

### Can I just cast a `struct file *` to get a file descriptor?

### Can I just cast a `struct thread *` to a `pid_t`?

No. You can directly cast them since `pid_t` and `file descriptor` are smaller
than the pointer type.

### Can I set a maximum number of open files per process?

It is better not to set an arbitrary limit. You may impose a limit of 128 open
files per process, if necessary. But if you want to implement extra
requirements, there should be no limitation.

### What happens when an open file is removed?

You should implement the standard Unix semantics for files. That is, when a
file is removed any process which has a file descriptor for that file may
continue to use that descriptor. This means that they can read and write from
the file. The file will not have a name, and no other processes will be able to
open it, but it will continue to exist until all file descriptors referring to
the file are closed or the machine shuts down.

### How can I run user programs that need more than 4 kB stack space?

You may modify the stack setup code to allocate more than one page of stack
space for each process. In the next project, you will implement a better
solution.
