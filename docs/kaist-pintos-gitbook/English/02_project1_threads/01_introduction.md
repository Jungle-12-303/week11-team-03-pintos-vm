# 01_introduction

# Project1: Threads

In this assignment, we give you a minimally functional thread system. Your job
is to extend the functionality of this system to gain a better understanding of
synchronization problems. You will be working primarily in the `threads`
directory for this assignment, with some work in the `devices` directory on the
side. Compilation should be done in the `threads` directory.
Before you read the description of this project, you should at least skim the
material [Synchronization](../06_appendix/02_synchronization.md).

## Background

### Understanding Threads

The first step is to read and understand the code for the initial thread system.
Pintos already implements thread creation and thread completion, a simple
scheduler to switch between threads, and synchronization primitives
(semaphores, locks, condition variables, and optimization barriers).

Some of this code might seem slightly mysterious. If you haven't already
compiled and run the base system, as described in the
[introduction](../01_INTRODUCTION/01_introduction.md) you should do so now.
You can read through parts of the source code to see what's going on. If you
like, you can add calls to `printf()` almost anywhere, then recompile and run to
see what happens and in what order. You can also run the kernel in a debugger
and set breakpoints at interesting spots, single-step through code and examine
data, and so on.

When a thread is created, you are creating a new context to be scheduled. You
provide a function to be run in this context as an argument to thread_create().
The first time the thread is scheduled and runs, it starts from the beginning of
that function and executes in that context. When the function returns, the
thread terminates. Each thread, therefore, acts like a mini-program running
inside Pintos, with the function passed to `thread_create()` acting like
`main()`.

At any given time, exactly one thread runs and the rest, if any, become
inactive. The scheduler decides which thread to run next. (If no thread is ready
to run at any given time, then the special *idle* thread, implemented in
`idle()`, runs.) Synchronization primitives can force context switches when one
thread needs to wait for another thread to do something.

The mechanics of a context switch are in `thread_launch()` in `threads/thread.c`
(You don't have to understand it.) It saves the state of the currently
running thread and restores the state of the thread we're switching to.

Using the GDB debugger, slowly trace through a context switch to see what
happens (see [GDB](../06_appendix/06_debugging_tools.md#gdb)). You can set a breakpoint on `schedule()` to start out,
and then single-step from there. Be sure to keep track of each thread's address
and state, and what procedures are on the call stack for each thread. You will
notice that when one thread executes `iret` in `do_iret()`, another thread
starts running.

**Warning**: In Pintos, each thread is assigned a small, fixed-size execution
stack just under 4 kB in size. The kernel tries to detect stack overflow, but it
cannot do so perfectly. You may cause bizarre problems, such as mysterious
kernel panics, if you declare large data structures as non-static local
variables, e.g. `int buf[1000];`. Alternatives to stack allocation include
the page allocator and the block allocator (see [Memory](../06_appendix/03_memory_allocation.md)​[Allocation](../06_appendix/03_memory_allocation.md).)

### Source Files

Here is a brief overview of the files in the `threads`, and `include/threads`
directory. You will not need to modify most of this code, but the hope is that
presenting this overview will give you a start on what code to look at.

#### `threads` codes

- `loader.S`, `loader.h`

  The kernel loader. Assembles to 512 bytes of code and data that the PC BIOS
  loads into memory and which in turn finds the kernel on disk, loads it into
  memory, and jumps to `bootstrap()` in `start.S`. You should not need to look
  at this code or modify it. `start.S` does basic setup needed for memory
  protection and jump into the 64bit long mode. Unlike the loader, this code is
  actually part of the kernel.
- `kernel.lds.S`

  The linker script used to link the kernel. Sets the load address of the kernel
  and arranges for `start.S` to be near the beginning of the kernel image.
  Again, you should not need to look at this code or modify it, but it's here in
  case you're curious.
- `init.c`, `init.h`

  Kernel initialization, including `main()`, the kernel's `main program.` You
  should look over `main()` at least to see what gets initialized. You might
  want to add your own initialization code here.
- `thread.c`, `thread.h`

  Basic thread support. Much of your work will take place in these files.
  `thread.h` defines struct thread, which you are likely to modify in all four
  projects. See [Threads](../06_appendix/01_threads.md) for more information.
- `palloc.c`, `palloc.h`

  Page allocator, which hands out system memory in multiples of 4 kB pages.
  See [Page Allocator](../06_appendix/03_memory_allocation.md#page-allocator) for more
  information.
- `malloc.c`, `malloc.h`

  A simple implementation of `malloc()` and `free()` for the kernel. See
  [Block Allocator](../06_appendix/03_memory_allocation.md#block-allocator) for more
  information.
- `interrupt.c`, `interrupt.h`

  Basic interrupt handling and functions for turning interrupts on and off.
- `intr-stubs.S`, `intr-stubs.h`

  Assembly code for low-level interrupt handling.
- `synch.c`, `synch.h`

  Basic synchronization primitives: semaphores, locks, condition variables, and
  optimization barriers. You will need to use these for synchronization in all
  four projects. See [Synchronization](../06_appendix/02_synchronization.md) for more
  information.
- `mmu.c`, `mmu.h`

  Functions for x86-64 page table operations. You will look this file closer
  after the lab1.
- `io.h`

  Functions for I/O port access. This is mostly used by source code in the
  `devices` directory that you won't have to touch.
- `vaddr.h`, `pte.h`

  Functions and macros for working with virtual addresses and page table
  entries. These will be more important to you in project 3. For now, you can
  ignore them.
- `flags.h`

  Macros that define a few bits in the x86-64 `flags` register. Probably of no
  interest.

#### `devices` codes

The basic threaded kernel also includes these files in the `devices` directory:

- `timer.c`, `timer.h`

  System timer that ticks, by default, 100 times per second. You will modify
  this code in this project.
- `vga.c`, `vga.h`

  VGA display driver. Responsible for writing text to the screen. You should
  have no need to look at this code. `printf()` calls into the VGA display
  driver for you, so there's little reason to call this code yourself.
- `serial.c`, `serial.h`

  Serial port driver. Again, `printf()` calls this code for you, so you don't
  need to do so yourself. It handles serial input by passing it to the input
  layer (see below).
- `block.c`, `block.h`

  An abstraction layer for block devices, that is, random-access, disk-like
  devices that are organized as arrays of fixed-size blocks. Out of the box,
  Pintos supports two types of block devices: IDE disks and partitions. Block
  devices, regardless of type, won't actually be used until project 2.
- `ide.c`, `ide.h`

  Supports reading and writing sectors on up to 4 IDE disks.
- `partition.c`, `partition.h`

  Understands the structure of partitions on disks, allowing a single disk to be
  carved up into multiple regions (partitions) for independent use.
- `kbd.c`, `kbd.h`

  Keyboard driver. Handles keystrokes passing them to the input layer
  (see below).
- `input.c`, `input.h`

  Input layer. Queues input characters passed along by the keyboard or serial
  drivers.
- `intq.c`, `intq.h`

  Interrupt queue, for managing a circular queue that both kernel threads and
  interrupt handlers want to access. Used by the keyboard and serial drivers.
- `rtc.c`, `rtc.h`

  Real-time clock driver, to enable the kernel to determine the current date and
  time. By default, this is only used by `thread/init.c` to choose an initial
  seed for the random number generator.
- `speaker.c`, `speaker.h`

  Driver that can produce tones on the PC speaker.
- `pit.c`, `pit.h`

  Code to configure the 8254 Programmable Interrupt Timer. This code is used
  by both `devices/timer.c` and `devices/speaker.c` because each device uses
  one of the PIT's output channel.

#### `lib` codes

Finally, `lib` and `lib/kernel` contain useful library routines. (`lib/user`
will be used by user programs, starting in project 2, but it is not part of the
kernel.) Here's a few more details:

- `ctype.h`, `inttypes.h`, `limits.h`, `stdarg.h`, `stdbool.h`, `stddef.h`,
  `stdint.h`, `stdio.c`, `stdio.h`, `stdlib.c`, `stdlib.h`, `string.c`,
  `string.h`

  A subset of the standard C library.
- `debug.c`, `debug.h`

  Functions and macros to aid debugging. See [Debugging Tools](../06_appendix/06_debugging_tools.md) for more
  information.
- `random.c`, `random.h`

  Pseudo-random number generator. The actual sequence of random values will
  not vary from one Pintos run to another.
- `round.h`

  Macros for rounding.
- `syscall-nr.h`

  System call numbers. Not used until project 2.
- `kernel/list.c`, `kernel/list.h`

  Doubly linked list implementation. Used all over the Pintos code, and you'll
  probably want to use it a few places yourself in project 1. **We recommand you**​**to skim this code before you start (especially comments in the header file.)**
- `kernel/bitmap.c`, `kernel/bitmap.h`

  Bitmap implementation. You can use this in your code if you like, but you
  probably won't have any need for it in project 1.
- `kernel/hash.c`, `kernel/hash.h`

  Hash table implementation. Likely to come in handy for project 3.
- `kernel/console.c`, `kernel/console.h`, `kernel/stdio.h`

  Implements `printf()` and a few other functions.

### Synchronization

Proper synchronization is an important part of the solutions to these problems.
Any synchronization problem can be easily solved by turning interrupts off:
while interrupts are off, there is no concurrency, so there's no possibility for
race conditions. Therefore, it's tempting to solve all synchronization problems
this way, but don't. Instead, use semaphores, locks, and condition variables to
solve the bulk of your synchronization problems. Read the tour section on
synchronization (see [Synchronization](../06_appendix/02_synchronization.md)) or the
comments in `threads/synch.c` if you're unsure what synchronization primitives
may be used in what situations.

In the Pintos projects, the only class of problem best solved by disabling
interrupts is coordinating data shared between a kernel thread and an interrupt
handler. Because interrupt handlers can't sleep, they can't acquire locks.
This means that data shared between kernel threads and an interrupt handler must
be protected within a kernel thread by turning off interrupts.

This project only requires accessing a little bit of thread state from
interrupt handlers. For the alarm clock, the timer interrupt needs to wake up
sleeping threads. In the advanced scheduler, the timer interrupt needs to access
a few global and per-thread variables. When you access these variables from
kernel threads, you will need to disable interrupts to prevent the timer
interrupt from interfering.

When you do turn off interrupts, take care to do so for the least amount of code
possible, or you can end up losing important things such as timer ticks or input
events. Turning off interrupts also increases the interrupt handling latency,
which can make a machine feel sluggish if taken too far.

The synchronization primitives themselves in `synch.c` are implemented by
disabling interrupts. You may need to increase the amount of code that runs with
interrupts disabled here, but you should still try to keep it to a minimum.

Disabling interrupts can be useful for debugging, if you want to make sure that
a section of code is not interrupted. You should remove debugging code before
turning in your project. (Don't just comment it out, because that can make the
code difficult to read.)

There should be no busy waiting in your submission. A tight loop that calls
`thread_yield()` is one form of busy waiting.

### Development Suggestions

In the past, many groups divided the assignment into pieces, then each group
member worked on his or her piece until just before the deadline, at which time
the group reconvened to combine their code and submit. This is a bad idea. We do
not recommend this approach. Groups that do this often find that two changes
conflict with each other, requiring lots of last-minute debugging. Some groups
who have done this have turned in code that did not even compile or boot,
much less pass any tests.

Instead, we recommend integrating your team's changes early and often, using a
source code control system such as [git](https://git-scm.com/docs/gittutorial).
This is less likely to produce surprises, because everyone can see everyone
else's code as it is written, instead of just when it is finished. These systems
also make it possible to review changes and, when a change introduces a bug,
drop back to working versions of code.

You should expect to run into bugs that you simply don't understand while
working on this and subsequent projects. When you do, reread the appendix on
debugging tools, which is filled with useful debugging tips that should help you
to get back up to speed (see [Debugging Tools](../06_appendix/06_debugging_tools.md)). Be sure to read the section
on backtraces (see [Backtraces](../06_appendix/06_debugging_tools.md#backtraces)), which will help you to get the most out of
every kernel panic or assertion failure.
