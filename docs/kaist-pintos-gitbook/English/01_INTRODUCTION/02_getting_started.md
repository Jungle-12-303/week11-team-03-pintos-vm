# 02_getting_started

# Getting Started

To get started, you'll have to log into a machine that Pintos can be built on.
You may use given Linux machines by the intructors or your own Solaris or Linux
machines. However, we will test your code on given machines, and the
instructions given here assume this environment. We cannot provide support for
installing and working on Pintos on your own machine. Belows server environment
will be given to each student.

Now you can get the source code for Pintos into a directory name 'pintos',
by executing:

```/bin/sh
git clone https://github.com/casys-kaist/pintos-kaist
```

However, the source codes are likely to be changed **before the project**​**period**, we recommend you to [duplicate the](https://help.github.com/en/github/creating-cloning-and-archiving-repositories/duplicating-a-repository)​[repository](https://help.github.com/en/github/creating-cloning-and-archiving-repositories/duplicating-a-repository).
When we change the template code, you can
[easily synchronize](https://help.github.com/en/github/collaborating-with-issues-and-pull-requests/syncing-a-fork)
your code with the template. Please **NOT TO MAKE PUBLIC FORK**.
We will never change the source code during the project period, except
for the critical bug lies on the template code.

FYI. We have tested the pintos code and the solution on the `Ubuntu 16.04.6 LTS`
with gcc (`gcc (Ubuntu 7.4.0-1ubuntu1~16.04~ppa1) 7.4.0`) and qemu-system-x86_64
(`QEMU emulator version 2.5.0 (Debian 1:2.5+dfsg-5ubuntu10.43)`), same as the
given linux machine.

Once you've retrived source code, you can set up your environment with below
command in pintos root directory:

```sh
$ source ./activate
```

It is a good idea to add this line to the '.bashrc' in your home directory.
Otherwise, you'll have to type it every time you log in.

## Source Tree Overview

Let's take a look at what's inside. Here's the directory structure that you
should see in `pintos/`

- `threads/`: Source code for the base kernel, which you will modify starting in
  project 1.
- `userprog/`: Source code for the user program loader, which you will modify
  starting with project 2.
- `vm/`: An almost empty directory. You will implement virtual memory here in
  project 3.
- `filesys/`: Source code for a basic file system. You will use this file system
  starting with project 2, but you will not modify it until project 4.
- `devices/`: Source code for I/O device interfacing: keyboard, timer, disk,
  etc. You will modify the timer implementation in project 1. Otherwise you
  should have no need to change this code.
- `lib/`: An implementation of a subset of the standard C library. The code in
  this directory is compiled into both the Pintos kernel and, starting from
  project 2, user programs that run under it. You should have little need to
  modify this code.
- `include/lib/kernel/`: Parts of the C library that are included only in the
  Pintos kernel. This also includes implementations of some data types that you
  are free to use in your kernel code: bitmaps, doubly linked lists, and
  hash tables. In the kernel, headers in this directory can be included using
  the `#include <...>` notation.
- `include/lib/user/`: Parts of the C library that are included only in Pintos
  user programs. In user programs, headers in this directory can be included
  using the `#include <...>` notation.
- `tests/`: Tests for each project. You can modify this code if it helps you
  test your submission, but we will replace it with the originals before we run
  the tests.
- `examples/`: Example user programs for use starting with project 2.
- `include/`: Source code for the header files (`*.h`).

## Building Pintos

As the next step, build the source code supplied for the first project. First,
`cd` into the `threads` directory. Then, issue the `make` command.
This will create a `build` directory under `threads`, populate it with a
`Makefile` and a few subdirectories, and then build the kernel inside.

Following the build, the following are the interesting files in the `build`
directory:

- `Makefile`: A copy of `pintos/src/Makefile.build`. It describes how to build
  the kernel.
- `kernel.o`: Object file for the entire kernel. This is the result of linking
  object files compiled from each individual kernel source file into a single
  object file. It contains debug information, so you can run GDB
  (see [GDB](../06_appendix/06_debugging_tools.md#gdb)) or backtrace
  ([Backtraces](../06_appendix/06_debugging_tools.md#backtraces)) on it.
- `kernel.bin`: Memory image of the kernel, that is, the exact bytes loaded into
  memory to run the Pintos kernel. This is just `kernel.o` with debug
  information stripped out, which saves a lot of space, which in turn keeps the
  kernel from bumping up against a 512 kB size limit imposed by the kernel
  loader's design.
- `loader.bin`: Memory image for the kernel loader, a small chunk of code
  written in assembly language that reads the kernel from disk into memory and
  starts it up. It is exactly 512 bytes long, a size fixed by the PC BIOS.
  Subdirectories of `build` contain object files (`.o`) and dependency files
  (`.d`), both produced by the compiler. The dependency files tell make which
  source files need to be recompiled when other source or header files are
  changed.

## Running Pintos

We've supplied a program for conveniently running Pintos in a simulator, called
pintos. In the simplest case, you can invoke pintos as pintos argument....
Each argument is passed to the Pintos kernel for it to act on.
Try it out. First cd into the newly created `build` directory. Then issue the
command pintos run alarm-multiple, which passes the arguments run alarm-multiple
to the Pintos kernel. In these arguments, run instructs the kernel to run a test
and alarm-multiple is the test to run. Pintos boots and runs the alarm-multiple
test program, which outputs a few screenfuls of text.

You can log serial output to a file by redirecting at the command line,
e.g. `pintos -- run alarm-multiple > logfile`. The pintos program offers several
options for configuring the qemu or the virtual hardware. If you specify any
options, they must precede the commands passed to the Pintos kernel and be
separated from them by `--`, so that the whole command looks like
`pintos option... -- argument....` Invoke pintos without any arguments to see a
list of available options. Options includes: how you want VM output to be
displayed: use `-v` to turn off the VGA display, or `-s` to suppress serial
input from stdin and output to stdout. The Pintos kernel has commands and
options other than run. These are not very interesting for now, but you can see
a list of them using `-h`, e.g. `pintos -h`.
