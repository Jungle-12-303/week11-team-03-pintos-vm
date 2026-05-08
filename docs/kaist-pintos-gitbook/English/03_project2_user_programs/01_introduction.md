# 01_introduction

# Project2: User Programs

Now that you've worked with Pintos and are becoming familiar with its
infrastructure and thread package, it's time to start working on the parts of
the system that allow running user programs. The base code already supports
loading and running user programs, but no I/O or interactivity is possible. In
this project, you will enable programs to interact with the OS via system calls.
You will be working out of the `userprog` directory for this assignment, but you
will also be interacting with almost every other part of Pintos. We will
describe the relevant parts below.

**You must build project 2 on top of your project 1 submission.**  Although no
code from project 1 affects code for project 2, you still have to pass test
cases for project 1 because this is an incremental project.

Also, please notify there is extra challenge from project2. **This is just an**​**optional implementation.**  Also, for the extra challenge, there isn't any
skeleton code for you except the test cases. All designs are up to you.
**To submit and test your extra requirements, you will need to edit**​**​`userprog/Make.vars`​**.

Lastly, please note that the code without `TODO` does not always you don't need
to change that code. You can freely modify any source codes in project2, except
the testing codes.

## Background

Up to now, all of the code you have run under Pintos has been part of the
operating system kernel. This means, for example, that all the test code from
the last assignment ran as part of the kernel, with full access to privileged
parts of the system. Once we start running user programs on top of the operating
system, this is no longer true. This project deals with the consequences.

We allow more than one process to run at a time. Each process has one thread
(multithreaded processes are not supported). User programs are written under the
illusion that they have the entire machine. This means that when you load and
run multiple processes at a time, you must manage memory, scheduling, and other
state correctly to maintain this illusion.

In the previous project, we compiled our test code directly into your kernel,
so we had to require certain specific function interfaces within the kernel.
From now on, we will test your operating system by running user programs. This
gives you much greater freedom. You must make sure that the user program
interface meets the specifications described here, but given that constraint you
are free to restructure or rewrite kernel code however you wish. All of your
codes should never located in block that enclosed by `#ifdef VM`. This blocks
will be included after enabling the virtual memory subsystem, which you will
implement in the project 3. Also, if the codes is enclosed by `#ifndef VM`,
those codes will be omitted from project3.

**We strongly recommend you to read**​**[synchronization](../06_appendix/02_synchronization.md)**​ ** and **​**[virtual](../06_appendix/04_virtual_address.md)**​**[addrees](../06_appendix/04_virtual_address.md)**​ ** before you start.**

### Source Files

The easiest way to get an overview of the programming you will be doing is to
simply go over each part you'll be working with. In `userprog`, you'll find a
small number of files, but here is where the bulk of your work will be:

- `process.c`, `process.h`

  Loads ELF binaries and starts processes.
- `syscall.c`, `syscall.h`

  Whenever a user process wants to access some kernel functionality, it invokes
  a system call. This is a skeleton system call handler. Currently, it just
  prints a message and terminates the user process. In part 2 of this project
  you will add code to do everything else needed by system calls.
- `syscall-entry.S`

  Small assembly codes that bootstrap the system call handler. You
  don't need to understand this code.
- `exception.c`, `exception.h`

  When a user process performs a privileged or prohibited operation, it traps
  into the kernel as an `exception` or `fault`. These files handle exceptions.
  Currently all exceptions simply print a message and terminate the process.
  Some, but not all, solutions to project 2 require modifying `page_fault()` in
  this file.
- `gdt.c`, `gdt.h`

  The x86-64 is a segmented architecture. The Global Descriptor Table (GDT)
  is a table that describes the segments in use. These files set up the GDT.
  You should not need to modify these files for any of the projects. You can
  read the code if you're interested in how the GDT works.
- `tss.c`, `tss.h`

  The Task-State Segment (TSS) was used for x86 architectural task switching.
  However, in x86-64, task switching is deprecated. Nontheless, TSS is still
  there to find the stack pointer during the ring switching.

  This means when a user process enters an interrupt handler, the hardware
  consults to the tss to look up the kernel's stack pointer. You should not need
  to modify these files for any of the pojects. You can read the code if you're
  interested in how the TSS works.

### Using the File System

You will need to interface to the file system code for this project, because
user programs are loaded from the file system and many of the system calls you
must implement deal with the file system. However, the focus of this project is
not the file system, so we have provided a simple but complete file system in
the `filesys` directory. You will want to look over the `filesys.h` and `file.h`
interfaces to understand how to use the file system, and especially its many
limitations.

**There is no need to modify the file system code for this project**, and so we
recommend that you do not. Working on the file system is likely to distract you
from this project's focus.

**Proper use of the file system routines now will make life much easier for**​**project 4**, when you improve the file system implementation. Until then, you
will have to tolerate the following limitations:

- No internal synchronization. Concurrent accesses will interfere with one
  another. You should use synchronization to ensure that only one process at a
  time is executing file system code.
- File size is fixed at creation time. The root directory is represented as a
  file, so the number of files that may be created is also limited.
- File data is allocated as a single extent, that is, data in a single file must
  occupy a contiguous range of sectors on disk. External fragmentation can
  therefore become a serious problem as a file system is used over time.
- No subdirectories
- File names are limited to 14 characters.
- A system crash mid-operation may corrupt the disk in a way that cannot be
  repaired automatically. There is no file system repair tool anyway.

**One important feature is included:**

Unix-like semantics for `filesys_remove()` are implemented. That is, if a file
is open when it is removed, its blocks are not deallocated and it may still be
accessed by any threads that have it open, until the last one closes it. See
[Removing an Open File](08_faq.md) for more information.

---

Unlike project 1 where all test programs were already present in the kernel image,
you have to put test programs (which runs in the user space) into the Pintos
virtual machine. To make your life much easier, our testing scripts
(i.e. `make check`) automatically handle this, so in most cases, you don't need
to understand this. However, knowing this would significantly help you to run
individual test cases.

To put a file into the Pintos virtual machine, firstly you need to be able to
create a simulated disk with a file system partition. The
`pintos-mkdisk` program provides this functionality. From the `userprog/build`
directory, execute `pintos-mkdisk filesys.dsk 2`. This command
creates a simulated disk named `filesys.dsk` that contains a 2 MB Pintos file
system partition. Then, specify the disk by passing `--fs-disk filesys.dsk`
after `pintos` but before `--` (i.e. `pintos --fs-disk filesys.disk -- KERNEL_COMMANDS...`).
The `--` is needed because `--fs-disk` is for the pintos script, not for
the simulated kernel. Afterward, format the file system partition by passing
`-f -q` on the kernel's command line: `pintos SCRIPT_COMMANDS -- -f -q`. The `-f`
option causes the file system to be formatted, and `-q` causes Pintos to exit
as soon as the format is done.

You'll need a way to copy files in and out of the simulated file system. The
pintos `-p` ("put") and `-g` ("get") options do this. To copy 'file' into the
Pintos file system, use the command `pintos -p file -- -q`. To copy it
to the Pintos file system under the name `newname`, add `:newname` after the
original filename: `pintos -p file:newname -- -q`. The commands for copying
files out of a VM are similar, but substitute `-g` for `-p`.

Incidentally, these commands work by passing special commands extract and append
on the kernel's command line and copying to and from a special simulated
"scratch" partition. If you're very curious, you can look at the pintos script
as well as `filesys/fsutil.c` to learn the implementation details.

Here's a summary of how to create a disk with a file system partition, format
the file system, copy the *args-single* program—which is the second test case for
this project—into the new disk, and then run it, passing argument 'onearg'.
(Argument passing won't work until you implemented it.) It assumes that you've
already built the test cases and that the current directory is `userprog/build`:

```sh
pintos-mkdisk filesys.dsk 10
pintos --fs-disk filesys.dsk -p tests/userprog/args-single:args-single -- -q -f run 'args-single onearg'
```

If you don't want to keep the file system disk around for later use or
inspection, you can even combine all four steps into a single command.
The `--filesys-size=n` option creates a temporary file system partition
approximately n megabytes in size just for the duration of the pintos run.
The Pintos automatic test suite makes extensive use of this syntax:

```sh
pintos --fs-disk=10 -p tests/userprog/args-single:args-single -- -q -f run 'args-single onearg'
```

### How User Programs Work

Pintos can run normal C programs, as long as they fit into memory and use only
the system calls you implement. Notably, `malloc()` cannot be implemented
because none of the system calls required for this project allow for memory
allocation. Pintos also can't run programs that use floating point operations,
since the kernel doesn't save and restore the processor's floating-point unit
when switching threads.

Pintos can load ELF executables with the loader provided for you in
`userprog/process.c`. ELF is a file format used by Linux, Solaris, and many
other operating systems for object files, shared libraries, and executables.

You can actually use any compiler and linker that output x86-64 ELF executables
to produce programs for Pintos. (We've provided compilers and linkers that
should do just fine.) You should realize immediately that, until you copy a test
program to the simulated file system, Pintos will be unable to do useful work.
You won't be able to do interesting things until you copy a variety of programs
to the file system. You might want to create a clean reference file system disk
and copy that over whenever you trash your `filesys.dsk` beyond a useful state,
which may happen occasionally while debugging.

### Virtual Memory Layout

Virtual memory in Pintos is divided into two regions: user virtual memory and
kernel virtual memory. User virtual memory ranges from virtual address `0` up to
`KERN_BASE`, which is defined in `include/threads/vaddr.h` and defaults to
`0x8004000000` Kernel virtual memory occupies the rest of the virtual address
space.

User virtual memory is per-process. When the kernel switches from one process to
another, it also switches user virtual address spaces by changing the
processor's page directory base register (see `pml4_activate()` in
`thread/mmu.c`). struct thread contains a pointer to a process's page table.

Kernel virtual memory is global. It is always mapped the same way, regardless
of what user process or kernel thread is running. In Pintos, kernel virtual
memory is mapped one-to-one to physical memory, starting at `KERN_BASE`. That
is, virtual address `KERN_BASE` accesses physical address `0`, virtual address
`KERN_BASE + 0x1234` accesses physical address `0x1234`, and so on up to the
size of the machine's physical memory.

A user program can only access its own user virtual memory. An attempt to access
kernel virtual memory causes a page fault, handled by `page_fault()` in
`userprog/exception.c`, and the process will be terminated. Kernel threads can
access both kernel virtual memory and, if a user process is running, the user
virtual memory of the running process. However, even in the kernel, an attempt
to access memory at an unmapped user virtual address will cause a page fault.

### Typical Memory Layout

Conceptually, each process is free to lay out its own user virtual memory
however it chooses. In practice, user virtual memory is laid out like this:

```
USER_STACK +----------------------------------+
           |             user stack           |
           |                 |                |
           |                 |                |
           |                 V                |
           |           grows downward         |
           |                                  |
           |                                  |
           |                                  |
           |                                  |
           |           grows upward           |
           |                 ^                |
           |                 |                |
           |                 |                |
           +----------------------------------+
           | uninitialized data segment (BSS) |
           +----------------------------------+
           |     initialized data segment     |
           +----------------------------------+
           |            code segment          |
 0x400000  +----------------------------------+
           |                                  |
           |                                  |
           |                                  |
           |                                  |
           |                                  |
       0   +----------------------------------+
```

In this project, the user stack is fixed in size, but in project 3 it will be
allowed to grow. Traditionally, the size of the uninitialized data segment can
be adjusted with a system call, but you will not have to implement this.

The code segment in Pintos starts at user virtual address 0x400000,
approximately 128 MB from the bottom of the address space. This value was
typical value in ubuntu and has no deep significance.

The linker sets the layout of a user program in memory, as directed by a "linker
script" that tells it the names and locations of the various program segments.
You can learn more about linker scripts by reading the "Scripts" chapter in the
linker manual, accessible via `info ld`.

To view the layout of a particular executable, run objdump with the `-p` option.

### Accessing User Memory

As part of a system call, the kernel must often access memory through pointers
provided by a user program. The kernel must be very careful about doing so,
because the user can pass a null pointer, a pointer to unmapped virtual memory,
or a pointer to kernel virtual address space (above `KERN_BASE`). All of these
types of invalid pointers must be rejected without harm to the kernel or other
running processes, by terminating the offending process and freeing its
resources.

There are at least two reasonable ways to do this correctly. The first method is
to verify the validity of a user-provided pointer, then dereference it. If you
choose this route, you'll want to look at the functions in `thread/mmu.c`
and in `include/threads/vaddr.h`. This is the simplest way to handle user
memory access.

The second method is to check only that a user pointer points below `KERN_BASE`,
then dereference it. An invalid user pointer will cause a "page fault" that you
can handle by modifying the code for `page_fault()` in `userprog/exception.c`.
This technique is normally faster because it takes advantage of the processor's
MMU, so it tends to be used in real kernels (including Linux).

In either case, you need to make sure not to "leak" resources. For example,
suppose that your system call has acquired a lock or allocated memory with
`malloc()`. If you encounter an invalid user pointer afterward, you must still
be sure to release the lock or free the page of memory. If you choose to verify
user pointers before dereferencing them, this should be straightforward. It's
more difficult to handle if an invalid pointer causes a page fault, because
there's no way to return an error code from a memory access. Therefore, for
those who want to try the latter technique, we'll provide a little bit of
helpful code:

```c
/* Reads a byte at user virtual address UADDR.
 * UADDR must be below KERN_BASE.
 * Returns the byte value if successful, -1 if a segfault
 * occurred. */
static int64_t
get_user (const uint8_t *uaddr) {
    int64_t result;
    __asm __volatile (
    "movabsq $done_get, %0\n"
    "movzbq %1, %0\n"
    "done_get:\n"
    : "=&a" (result) : "m" (*uaddr));
    return result;
}

/* Writes BYTE to user address UDST.
 * UDST must be below KERN_BASE.
 * Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte) {
    int64_t error_code;
    __asm __volatile (
    "movabsq $done_put, %0\n"
    "movb %b2, %1\n"
    "done_put:\n"
    : "=&a" (error_code), "=m" (*udst) : "q" (byte));
    return error_code != -1;
}
```

Each of these functions assumes that the user address has already been verified
to be below `KERN_BASE`. They also assume that you've modified `page_fault()` so
that a page fault in the kernel merely sets rax to -1 and copies its former
value into `%rip`.
