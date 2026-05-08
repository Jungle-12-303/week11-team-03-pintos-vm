# 06_debugging_tools

# Debugging Tools

Many tools lie at your disposal for debugging Pintos. This appendix introduces
you to a few of them.

## `printf()`

Don't underestimate the value of `printf()`. The way `printf()` is implemented
in Pintos, you can call it from practically anywhere in the kernel, whether it's
in a kernel thread or an interrupt handler, almost regardless of what locks are
held. `printf()` is useful for more than just examining data. It can also help
figure out when and where something goes wrong, even when the kernel crashes or
panics without a useful error message. The strategy is to sprinkle calls to
`printf()` with different strings (e.g. "<1>", "<2>", . . . ) throughout the
pieces of code you suspect are failing. If you don't even see <1> printed, then
something bad happened before that point, if you see <1> but not <2>, then
something bad happened between those two points, and so on. Based on what you
learn, you can then insert more `printf()` calls in the new, smaller region of
code you suspect. Eventually you can narrow the problem down to a single
statement. See [Triple Faults](06_debugging_tools.md#triple-faults),
for a related technique.

## ASSERT

Assertions are useful because they can catch problems early, before they'd
otherwise be noticed. Ideally, each function should begin with a set of
assertions that check its arguments for validity. (Initializers for functions'
local variables are evaluated before assertions are checked, so be careful not
to assume that an argument is valid in an initializer.) You can also sprinkle
assertions throughout the body of functions in places where you suspect things
are likely to go wrong. They are especially useful for checking loop invariants.

Pintos provides the `ASSERT` macro, defined in `<debug.h>`, for checking
assertions.

```c
#define ASSERT(expression) { /* Omit details */ }
```

Tests the value of expression. If it evaluates to zero (false), the kernel
panics. The panic message includes the expression that failed, its file and
line number, and a backtrace, which should help you to find the problem. See
[Backtraces](#backtraces) for more information.

## Function and Parameter Attributes

These macros defined in `<debug.h>` tell the compiler special attributes of a
function or function parameter. Their expansions are GCC-specific.

---

```c
#define UNUSED { /* Omit details */ }
```

> Appended to a function parameter to tell the compiler that the parameter might
> not be used within the function. It suppresses the warning that would
> otherwise appear.

---

```c
#define NO_RETURN { /* Omit details */ }
```

> Appended to a function prototype to tell the compiler that the function never
> returns. It allows the compiler to fine-tune its warnings and its code
> generation.

---

```c
#define NO_INLINE { /* Omit details */ }
```

> Appended to a function prototype to tell the compiler to never emit the
> function in-line. Occasionally useful to improve the quality of backtraces
> (see below).

---

```c
#define PRINTF_FORMAT(format, first) { /* Omit details */ }
```

> Appended to a function prototype to tell the compiler that the function takes
> a `printf()`-like format string as the argument numbered format (starting from
>
> 1) and that the corresponding value arguments start at the argument numbered
>    first. This lets the compiler tell you if you pass the wrong argument types.

## Backtraces

When the kernel panics, it prints a "backtrace," that is, a summary of how your
program got where it is, as a list of addresses inside the functions that were
running at the time of the panic. You can also insert a call to
`debug_backtrace()`, prototyped in `<debug.h>`, to print a backtrace at any
point in your code. `debug_backtrace_all()`, also declared in `<debug.h>`,
prints backtraces of all threads.

The addresses in a backtrace are listed as raw hexadecimal numbers, which are
difficult to interpret. We provide a tool called `backtrace` to translate these
into function names and source file line numbers. Give it the name of your
`kernel.o` as the first argument and the hexadecimal numbers composing the
backtrace (including the `0x` prefixes) as the remaining arguments. It outputs
the function name and source file line numbers that correspond to each address.

If the translated form of a backtrace is garbled, or doesn't make sense (e.g.
function A is listed above function B, but B doesn't call A), then it's a good
sign that you're corrupting a kernel thread's stack, because the backtrace is
extracted from the stack. Alternatively, it could be that the `kernel.o` you
passed to backtrace is not the same kernel that produced the backtrace.

Sometimes backtraces can be confusing without any corruption. Compiler
optimizations can cause surprising behavior. When a function has called another
function as its final action (a tail call), the calling function may not appear
in a backtrace at all. Similarly, when function A calls another function B that
never returns, the compiler may optimize such that an unrelated function C
appears in the backtrace instead of A. Function C is simply the function that
happens to be in memory just after A. In the threads project, this is commonly
seen in backtraces for test failures.

### Example

Here's an example. Suppose that Pintos printed out this following call stack,
which is taken from an actual Pintos submission for the file system project:

```bash
Call stack: 0x800426eff 0x8004202fb 0x80042dc22 0x80042cf67 0x800422319
0x80042325a 0x40012c 0x400a96 0x400ac8.
```

You would then invoke the `backtrace` utility like shown below, cutting and
pasting the backtrace information into the command line. This assumes that
`kernel.o` is in the current directory. You would of course enter all of the
following on a single shell command line, even though that would overflow our
margins here:

```bash
backtrace 0x800426eff 0x8004202fb 0x80042dc22 0x80042cf67 0x800422319
0x80042325a 0x400012c 0x400a96 0x4000ac8
```

The backtrace output would then look something like this:

```bash
0x0000000800426eff: debug_panic (lib/debug.c:86)
0x00000008004202fb: file_seek (filesys/file.c:405)
0x000000080042dc22: seek (userprog/syscall.c:744)
0x000000080042cf67: syscall_handler (userprog/syscall.c:444)
0x0000000800422319: intr_handler (threads/interrupt.c:334)
0x000000080042325a: intr_entry (threads/intr-stubs.S:38)
0x000000000040012c: (unknown)
0x0000000000400a96: (unknown)
0x0000000000400ac8: (unknown)
```

(You will probably not see exactly the same addresses if you run the command
above on your own kernel binary, because the source code you compiled and the
compiler you used are probably different.)
The first line in the backtrace refers to `debug_panic()`, the function that
implements kernel panics. Because backtraces commonly result from kernel panics,
`debug_panic()` will often be the first function shown in a backtrace.
The second line shows `file_seek()` as the function that panicked, in this case
as the result of an assertion failure. In the source code tree used for this
example, line 405 of `filesys/file.c` is the assertion

```
ASSERT (file_ofs >= 0);
```

(This line was also cited in the assertion failure message.) Thus, `file_seek()`
panicked because it passed a negative file offset argument.
The third line indicates that `seek()` called `file_seek()`, presumably without
validating the offset argument. In this submission, `seek()` implements the seek
system call. The fourth line shows that `syscall_handler()`, the system call
handler, invoked `seek()`. The fifth and sixth lines are the interrupt handler
entry path. The remaining lines are for addresses below `KERN_BASE`. This means
that they refer to addresses in the user program, not in the kernel.

## GDB

You can run Pintos under the supervision of the GDB debugger. First, start
Pintos with the `--gdb` option, e.g. `pintos --gdb -- run mytest`. Second, open
a second terminal on the same machine and use gdb to invoke GDB on 'kernel.o'

```
gdb kernel.o
```

and issue the following GDB command:

```
target remote localhost:1234
```

Now GDB is connected to the simulator over a local network connection. You can
now issue any normal GDB commands. If you issue the 'c' command, the simulated
BIOS will take control, load Pintos, and then Pintos will run in the usual way.
You can pause the process at any point with Ctrl+C.

### Using GDB

You can read the GDB manual by typing `info gdb` at a terminal command prompt.
Here's a few commonly useful GDB commands:

- c [GDB Command]

> Continues execution until hCtrl+Ci or the next breakpoint.

- break *function* [GDB Command]
- break *file:line* [GDB Command]
- break *address* [GDB Command]

> Sets a breakpoint at function, at line within file, or address. (Use a '0x'
> prefix to specify an address in hex.)
> Use break main to make GDB stop when Pintos starts running.

- p *expression* [GDB Command]

> Evaluates the given expression and prints its value. If the expression
> contains a function call, that function will actually be executed.

- l *address* [GDB Command]

> Lists a few lines of code around address. (Use a '0x' prefix to specify an
> address in hex.)

- bt [GDB Command]

> Prints a stack backtrace similar to that output by the backtrace program
> described above.

- p/a *address* [GDB Command]

> Prints the name of the function or variable that occupies address. (Use a
> '0x' prefix to specify an address in hex.)

- diassemble *function* [GDB Command]

> Disassembles function.

Here's an extra tip for anyone who read this: install gdb script such as
[pwndbg](https://github.com/pwndbg/pwndbg). This makes your life easier!

### FAQ

- Can I use GDB inside Emacs?

> Yes, you can. Emacs has special support for running GDB as a subprocess.
> Type M-x gdb and enter your pintos-gdb command at the prompt. The Emacs
> manual has information on how to use its debugging features in a section
> titled "Debuggers."

- GDB is doing something weird.

> If you notice strange behavior while using GDB, there are three possibilities:
> a bug in your modified Pintos, a bug in Bochs's interface to GDB or in GDB
> itself, or a bug in the original Pintos code. The first and second are quite
> likely, and you should seriously consider both. We hope that the third is less
> likely, but it is also possible.

## Triple Faults

When a CPU exception handler, such as a page fault handler, cannot be invoked
because it is missing or defective, the CPU will try to invoke the "double
fault" handler. If the double fault handler is itself missing or defective,
that's called a "triple fault." A triple fault causes an immediate CPU reset.

Thus, if you get yourself into a situation where the machine reboots in a loop,
that's probably a "triple fault." In a triple fault situation, you might not be
able to use `printf()` for debugging, because the reboots might be happening
even before everything needed for `printf()` is initialized.

There exists way to debug triple faults.

Pick a place in the Pintos code, insert the infinite loop `for (;;);` there, and
recompile and run. There are two likely possibilities:

- The machine hangs without rebooting. If this happens, you know that the
  infinite loop is running. That means that whatever caused the reboot must be
  *after* the place you inserted the infinite loop. Now move the infinite loop
  later in the code sequence.
- The machine reboots in a loop. If this happens, you know that the machine
  didn't make it to the infinite loop. Thus, whatever caused the reboot must be
  *before* the place you inserted the infinite loop. Now move the infinite loop
  earlier in the code sequence.

  If you move around the infinite loop in a "binary search" fashion, you can use
  this technique to pin down the exact spot that everything goes wrong. It
  should only take a few minutes at most.

## Tips

The page allocator in `threads/palloc.c` and the block allocator in
`threads/malloc.c` clear all the bytes in memory to '0xcc' at time of free.
Thus, if you see an attempt to dereference a pointer like `0xcccccccc`, or some
other reference to `0xcc`, there's a good chance you're trying to reuse a page
that's already been freed. Also, byte 0xcc is the CPU opcode for "invoke
interrupt 3," so if you see an error like `Interrupt 0x03 (#BP Breakpoint Exception)`, then Pintos tried to execute code in a freed page or block. An
assertion failure on the expression `sec_no < d->capacity` indicates that Pintos
tried to access a file through an inode that has been closed and freed. Freeing
an inode clears its starting sector number to `0xcccccccc`, which is not a valid
sector number for disks smaller than about 1.6 TB.
