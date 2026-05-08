# 01_introduction

# Project 4: File Systems

In the previous two assignments, you made extensive use of a file system without
actually worrying about how it was implemented underneath. For this last
assignment, you will improve the implementation of the file system. You will be
working primarily in the `filesys` directory.

You may build project 4 on top of project 2 or project 3. In either case, all of
the functionality needed for project 2 must work in your filesys submission. If
you build on project 3, then all of the project 3 functionality must work also,
and you will need to edit `filesys/Make.vars` to enable VM functionality. We
will deduct 10% credit if you disable the VM.
Note that the page cache (extra credit) requires the VM functionality.

## Background

### New Code

Here are some files that are probably new to you. These are in the `filesys` directory
except where indicated:

- `filesys/fsutil.c`

  > Simple utilities for the file system that are accessible from the kernel command line.
  >
- `include/filesys/filesys.h`, `filesys/filesys.c`

  > Top-level interface to the file system.
  > See [Using the File System](../03_project2_user_programs/01_introduction.md), for an introduction.
  >
- `include/filesys/directory.h`, `filesys/directory.c`

  > Translates file names to inodes. The directory data structure is stored as a file.
  >
- `include/filesys/inode.h`, `filesys/inode.c`

  > Manages the data structure representing the layout of a files data on disk.
  >
- `include/filesys/fat.h`, `filesys/fat.c`

  > Manage the FAT filesystem.
  >
- `include/filesys/file.h`, `filesys/file.c`

  > Translates file reads and writes to disk sector reads and writes.
  >
- `include/filesys/page_cache.h`, `filesys/page_cache.c`

  > Page cache implementation that utilize vm functionalities. We strongly
  > recommend you to use this template. But you might write down your own codes.
  > Note that if you turn off the vm flag, you cannot use this template.
  >
- `include/lib/kernel/bitmap.h`, `lib/kernel/bitmap.c`

  > A bitmap data structure along with routines for reading and writing the bitmap to disk files.
  >

Our file system has a Unix-like interface, so you may also wish to read the Unix
man pages for `creat, open, close, read, write, lseek`, and `unlink`. Our file
system has calls that are similar, but not identical, to these. The file system
translates these calls into disk operations.

All the basic functionality is there in the code above, so that the file system is usable from
the start, as you've seen in the previous two projects. However, it has severe limitations
which you will remove.

While most of your work will be in `filesys`, you should be prepared for interactions
with all previous parts.

#### Testing File System Persistence

Until now, each test invoked Pintos just once. However, an important purpose of a
file system is to ensure that data remains accessible from one boot to another. Thus, the
tests that are part of the file system project invoke Pintos a second time. The second run
combines all the files and directories in the file system into a single file, then copies that file
out of the Pintos file system into the host (Unix) file system.

The grading scripts check the file systems correctness based on the contents of the file
copied out in the second run. This means that your project will not pass any of the extended
file system tests until the file system is implemented well enough to support `tar`, the Pintos
user program that produces the file that is copied out. The `tar` program is fairly demanding
(it requires both extensible file and subdirectory support), so this will take some work. Until
then, you can ignore errors from `make check` regarding the extracted file system.

Incidentally, as you may have surmised, the file format used for copying out the file
system contents is the standard Unix "tar" format. You can use the Unix tar program to
examine them. The tar file for test t is named `t.tar`.
