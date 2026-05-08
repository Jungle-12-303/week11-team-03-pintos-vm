# 01_introduction

# Project3: Virtual Memory

By now you should have some familiarity with the inner workings of Pintos. Your
OS can properly handle multiple threads of execution with proper
synchronization, and can load multiple user programs at once. However, the
number and size of programs that can run is limited by the machine's main memory
size. In this assignment, you will remove that limitation by building an
illusion of infinite memory.

You will build this assignment on top of the last one. Test programs from
project 2 should also work with project 3. You should take care to fix any bugs
in your project 2 submission before you start work on project 3, because those
bugs will most likely cause the same problems in project 3.

For the project 3, we make step-by-step directions for your ease.

## Background

### Source Files

You will work in the `vm` directory for this project. The `Makefile` is updated
to turn on the setting `-DVM`. We provide an enormous amount of template code.
***You MUST follow the given template. That is, if you submit the code, that***​***is not based on the given template, you get 0pts.***  Also, you should
never change the template where it is marked "DO NOT CHANGE". Here, we
provide some details about each template file that you will be modifying.

- `include/vm/vm.h`, `vm/vm.c`

  Provides a general interface for virtual memory. In the header file,
  you can see the defintion and explanation for different vm_type -- VM_UNINIT,
  VM_ANON, VM_FILE, VM_PAGE_CACHE -- that your virtual memory system has to support (ignore VM_PAGE_CACHE for now, this is for project 4).
  You will also implement your supplementary page table here (read below).
- `include/vm/uninit.h`, `vm/uninit.c`

  Provides operations for uninitialized pages (vm_type = `VM_UNINIT`). Under
  the current design, all pages are initially set up as uninitialized pages,
  then it transforms to anonymous pages or file-backed pages.
- `include/vm/anon.h`, `vm/anon.c`

  Provides operations for anonymous pages (vm_type = `VM_ANON`).
- `include/vm/file.h`, `vm/file.c`

  Provides operations for file-backed pages (vm_type = `VM_FILE`).
- `include/vm/inspect.h`, `vm/inspect.c`

  Contains memory inspection operations for grading. Do not change this files.

Most of the code you write for this project will be files
in `vm` directory and in files introduced in earlier projects.
You will probably be encountering just a few files for the first time such as:

- `include/devices/block.h`, `devices/block.c`

  Provides sector-based read and write access to block device. You will use this
  interface to access the swap partition as a block device.

### Memory Terminology

We begin by presenting some terminology for memory and storage.
Some of these terms should be familiar from project 2 (see
[Virtual Memory Layout](../03_project2_user_programs/01_introduction.md)), but much of it is new.

#### Pages

A page, sometimes called a virtual page, is a continuous region of virtual
memory of size 4,096 bytes (the ***page size***) in length. A page must be
***page-aligned***, that is, start on a virtual address evenly divisible by the
page size. Thus, the last 12 bits of a 64-bit virtual address is the
***page offset*** (or just ***offset***). The upper bits are used to indicate
the index in the page table, which will be introduced soon. With 64-bit system,
we use 4-level page table, which makes a virtual address to look like this:

```
63          48 47            39 38            30 29            21 20         12 11         0
+-------------+----------------+----------------+----------------+-------------+------------+
| Sign Extend |    Page-Map    | Page-Directory | Page-directory |  Page-Table |    Page    |
|             | Level-4 Offset |    Pointer     |     Offset     |   Offset    |   Offset   |
+-------------+----------------+----------------+----------------+-------------+------------+
              |                |                |                |             |            |
              +------- 9 ------+------- 9 ------+------- 9 ------+----- 9 -----+---- 12 ----+
                                          Virtual Address
```

Each process has an independent set of user (virtual) pages, which are those
pages below the virtual address `KERN_BASE` (0x8004000000). The set of
***kernel (virtual) pages***, on the other hand, is global, and thus remain in
the same position
regardless of what thread or process is running. The kernel may access both user
and kernel pages, but a user process may access only its own user pages. See
[Virtual Memory Layout](../03_project2_user_programs/01_introduction.md), for more information.

Pintos provides several useful functions for working with virtual addresses.
See Section [Virtual Addresses](../06_appendix/04_virtual_address.md), for details.

#### Frames

A ***frame***, sometimes called a ***physical frame*** or a ***page frame***, is
a continuous region of physical memory. Like pages, frames must be page-size and
page-aligned. Thus, a 64-bit physical address can be divided into a
***frame number*** and a frame ***offset*** (or just ***offset***), like this:

```
                      12 11         0
+-----------------------+-----------+
|      Frame Number     |   Offset  |
+-----------------------+-----------+
          Physical Address
```

x86-64 doesn't provide any way to directly access memory at a physical
address. Pintos works around this by mapping kernel virtual memory directly to
physical memory - the first page of kernel virtual memory is mapped to the first
frame of physical memory, the second page to the second frame, and so on. Thus,
frames can be accessed through kernel virtual memory.

Pintos provides functions for translating between physical addresses and kernel
virtual addresses. See [Virtual Addresses](../06_appendix/04_virtual_address.md) for
details.

#### Page Tables

A ***page table*** is a data structure that the CPU uses to translate a virtual
address to a physical address, that is, from a page to a frame. The page table
format is dictated by the x86-64 architecture. Pintos provides page table
management code in `threads/mmu.c`.

The diagram below illustrates the relationship between pages and frames. The
virtual address, on the left, consists of a page number and an offset. The page
table translates the page number into a frame number, which is combined with the
unmodified offset to obtain the physical address, on the right.

```
                      +----------+
     .--------------->|Page Table|-----------.
    /                 +----------+            |
    |   12 11 0                               V  12 11 0
+---------+----+                         +---------+----+
| Page Nr | Ofs|                         |Frame Nr | Ofs|
+---------+----+                         +---------+----+
 Virt Addr   |                            Phys Addr    ^
              \_______________________________________/
```

#### Swap Slots

A ***swap slot*** is a page-size region of disk space in the swap
partition. Although hardware limitations dictating the placement
of slots are more flexible than for frames, swap slots should be
page-aligned because there is no downside in doing so.

### Resource Management Overview

You will need to design/implement the following data structures:

***Supplemental page table***

> Enables page fault handling by supplementing the page table.
> See Managing the Supplemental Page Table below.

***Frame table***

> Allows efficient implementation of eviction policy of physical frames
> See Managing the Frame Table below.

***Swap table***

> Tracks usage of swap slots. See Managing the Swap Table below.

You do not necessarily need to implement three completely distinct data
structures: it may be convenient to wholly or partially merge related resources
into a unified data structure.

For each data structure, you need to determine what information each element
should contain. You also need to decide on the data structure's scope, either
local (per-process) or global (applying to the whole system), and how many
instances are required within its scope.

To simplify your design, you may store these data structures in non-pageable
memory (e.g., memory allocated by `calloc` or `malloc`). That means that you can be sure that pointers among them will remain valid.

#### Choices of implementation (Performance perspective)

Possible choices for implementation include arrays, lists, bitmaps, and hash
tables. An array is often the simplest approach, but a sparsely populated array
wastes memory. Lists are also simple, but traversing a long list to find
a particular position wastes time. Both arrays and lists can be resized, but lists
more efficiently support insertion and deletion in the middle.

Pintos includes a bitmap data structure in `lib/kernel/bitmap.c` and
`include/lib/kernel/bitmap.h`. A bitmap is an array of bits, each of which can be true
or false. Bitmaps are typically used to track usage in a set of (identical)
resources: if resource n is in use, then bit ***n*** of the bitmap is true.
Pintos bitmaps are fixed in size, although you could extend their implementation
to support resizing.

Pintos also includes a hash table data structure (See
[Hash Table](../06_appendix/08_hash_table.md)). Pintos hash tables efficiently support
insertions and deletions over a wide range of table sizes.

Although more complex data structures may yield better performance or other benefits,
they may also needlessly complicate your implementation. Thus, we do not
recommend implementing any advanced data structure (e.g. a balanced binary tree)
as part of your design.

### Managing the Supplemental Page Table

The ***supplemental*** page table supplements the page table with additional
data about each page. It is needed because of the limitations imposed by the
page table's format. Such a data structure is often called a "page table" also;
we add the word "supplemental" to reduce confusion.

The supplemental page table is used for at least two purposes. Most importantly,
on a page fault, the kernel looks up the virtual page that faulted in the
supplemental page table to find out what data should be there. Second, the
kernel consults the supplemental page table when a process terminates, to decide
what resources to free.

#### Organization of Supplemental Page Table

You may organize the supplemental page table as you wish. There are at least two
basic approaches to its organization: in terms of segments or in terms of pages.
A segment here refers to a consecutive group of pages, i.e., memory region
containing an executable or a memory-mapped file.

Optionally, you may use the page table itself to track the members
of the supplemental page table. You will have to modify the Pintos page table
implementation in `threads/mmu.c` to do so. We recommend this approach for
advanced students only.

#### Handling page fault

The most important user of the supplemental page table is the page fault
handler. In project 2, a page fault always indicated a bug in the kernel or a
user program. In project 3, this is no longer true. Now, a page fault might only
indicate that the page must be brought in from a file or swap slot. You will have to
implement a more sophisticated page fault handler to handle these cases. The
page fault handler, which is `page_fault()` in `userprog/exception.c`, calls
your page fault handler, `vm_try_handle_fault()` in `vm/vm.c`.
Your page fault handler needs to do roughly the following:

1. Locate the page that faulted in the supplemental page table. If the memory
   reference is valid, use the supplemental page table entry to locate the data
   that goes in the page, which might be in the file system, or in a swap slot, or
   it might simply be an all-zero page. If you implement sharing (i.e., Copy-on-Write), the page's data
   might even already be in a page frame, but not in the page table.
   If the supplemental page table indicates that the user process should not
   expect any data at the address it was trying to access, or if the page lies
   within kernel virtual memory, or if the access is an attempt to write to a
   read-only page, then the access is invalid. Any invalid access terminates the
   process and thereby frees all of its resources.
2. Obtain a frame to store the page. If you implement sharing, the data you need
   may already be in a frame, in which case you must be able to locate that frame.
3. Fetch the data into the frame, by reading it from the file system or swap,
   zeroing it, etc. If you implement sharing, the page you need may already be in a
   frame, in which case no action is necessary in this step.
4. Point the page table entry for the faulting virtual address to the physical
   page. You can use the functions in `threads/mmu.c`.

### Managing the Frame Table

The frame table contains one entry for each frame.
Each entry in the frame table contains a pointer to the page, if any, that
currently occupies it, and other data of your choice. The frame table allows
Pintos to efficiently implement an eviction policy, by choosing a page to evict
when no frames are free.

The frames used for user pages should be obtained from the "user pool," by
calling `palloc_get_page(PAL_USER)`. You must use `PAL_USER` to avoid allocating
from the "kernel pool," which could cause some test cases to fail unexpectedly.
If you modify `palloc.c` as part of your frame table implementation, be sure to
retain the distinction between the two pools.

The most important operation on the frame table is obtaining an unused frame.
This is easy when a frame is free. When none is free, a frame must be made free
by evicting some page from its frame.

If no frame can be evicted without allocating a swap slot, but swap is full,
panic the kernel. Real OSes apply a wide range of policies to recover from or
prevent such situations, but these policies are beyond the scope of this
project.

The process of eviction comprises roughly the following steps:

1. Choose a frame to evict, using your page replacement algorithm. The
   "accessed" and "dirty" bits in the page table, described below, will come in
   handy.
2. Remove references to the frame from any page table that refers to it. Unless
   you have implemented sharing, only a single page should refer to a frame at any
   given time.
3. If necessary, write the page to the file system or to swap. The evicted frame
   may then be used to store a different page.

#### Accessed and Dirty Bits

x86-64 hardware provides some assistance for implementing page replacement
algorithms, through a pair of bits in the page table entry (PTE) for each page.
On any read or write to a page, the CPU sets the accessed bit to 1 in the page's
PTE, and on any write, the CPU sets the ***dirty bit*** to 1. The CPU never
resets these bits to 0, but the OS may do so.

You need to be aware of `aliases`, that is, two (or more) pages that refer to
the same frame. When an aliased frame is accessed, the accessed and dirty bits
are updated in only one page table entry (the one for the page used for access).
The accessed and dirty bits for the other aliases are not updated.

In Pintos, every user virtual page is aliased to its kernel virtual page. You
must manage these aliases somehow. For example, your code could check and update
the accessed and dirty bits for both addresses. Alternatively, the kernel could
avoid the problem by only accessing user data through the user virtual address.

Other aliases should only arise if you implement sharing or if
there is a bug in your code.

See Section [Page Table Accessed and Dirty Bits](../06_appendix/05_page_table.md) for
details of the functions to work with accessed and dirty bits.

### Managing the Swap Table

The swap table tracks in-use and free swap slots. It should allow picking an
unused swap slot for evicting a page from its frame to the swap partition. It
should allow freeing a swap slot when its page is read back or the process whose
page was swapped is terminated.

From the
`vm/build` directory, use the command `pintos-mkdisk swap.dsk --swap-size=n` to
create a disk named `swap.dsk` that contains a n-MB swap partition. Afterward,
`swap.dsk` will automatically be attached as an extra disk when you run pintos.
Alternatively, you can tell pintos to use a temporary n-MB swap disk for a
single run with `--swap-size=n`.

Swap slots should be allocated lazily, that is, only when they are actually
required by eviction. Reading data pages from the executable and writing them to
swap immediately at process startup is not lazy. Swap slots should not be
reserved to store particular pages.

Free a swap slot when its contents are read back into a frame.

### Managing Memory Mapped Files

The file system is most commonly accessed with `read` and `write` system calls.
A secondary interface is to "map" the file into virtual pages, using the `mmap`
system call. The program can then use memory instructions directly on the file
data. Suppose file `foo` is `0x1000` bytes (4 kB, or one page) long. If `foo` is
mapped into memory starting at address `0x5000`, then any memory accesses to
locations `0x5000. . .0x5fff` will access the corresponding bytes of `foo`.

Here's a program that uses `mmap` to print a file to the console. It opens the
file specified on the command line, maps it at virtual address `0x10000000`,
writes the mapped data to the console (fd 1), and unmaps the file.

```c
#include <stdio.h>
#include <syscall.h>
int main (int argc UNUSED, char *argv[])
{
  void *data = (void *) 0x10000000;                 /* Address at which to map. */
  int fd = open (argv[1]);                          /* Open file. */
  void *map = mmap (data, filesize (fd), 0, fd, 0); /* Map file. */
  write (1, data, filesize (fd));                   /* Write file to console. */
  munmap (map);                                     /* Unmap file (optional). */
  return 0;
}
```

Your submission must be able to track what memory is used by memory mapped
files. This is necessary to properly handle page faults in the mapped regions
and to ensure that mapped files do not overlap any other segments within the
process.
