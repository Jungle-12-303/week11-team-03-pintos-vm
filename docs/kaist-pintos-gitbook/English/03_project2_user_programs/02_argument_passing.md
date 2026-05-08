# 02_argument_passing

### Argument Passing

**Setup the argument for user program in **​**​`process_exec()`​**

#### x86-64 Calling Convention

This section summarizes important points of the convention used for normal
function calls on 64-bit x86-64 implementations of Unix. Some details are
omitted for brevity. For more detail, you can refer
[System V AMD64 ABI](https://en.wikipedia.org/wiki/X86_calling_conventions#System_V_AMD64_ABI).

The calling convention works like this:

1. User-level applications use as integer registers for passing the sequence
   `%rdi`, `%rsi`, `%rdx`, `%rcx`, `%r8` and `%r9`.
2. The caller pushes the address of its next instruction (the return address) on
   the stack and jumps to the first instruction of the callee. A single x86-64
   instruction, `CALL`, does both.
3. The callee executes.
4. If the callee has a return value, it stores it into register RAX.
5. The callee returns by popping the return address from the stack and jumping
   to the location it specifies, using the x86-64 `RET` instruction.

Consider a function `f()` that takes three int arguments. This diagram shows a
sample stack frame and register state as seen by the callee at the beginning of
step 3 above, supposing that `f()` is invoked as `f(1, 2, 3)`.
The initial stack address is arbitrary:

```
                             +----------------+
stack pointer --> 0x4747fe70 | return address |
                             +----------------+
RDI: 0x0000000000000001 | RSI: 0x0000000000000002 | RDX: 0x0000000000000003
```

#### Program Startup Details

The Pintos C library for user programs designates `_start()`, in
`lib/user/entry.c`, as the entry point for user programs. This function is a
wrapper around `main()` that calls `exit()` if `main()` returns:

```c
void
_start (int argc, char *argv[]) {
    exit (main (argc, argv));
}
```

The kernel must put the arguments for the initial function on the register
before it allows the user program to begin executing. The arguments are passed
in the same way as the normal calling convention.

Consider how to handle arguments for the following example command:
`/bin/ls -l foo bar`.

1. Break the command into words: `/bin/ls`, `-l`, `foo`, `bar`.
2. Place the words at the top of the stack. Order doesn't matter,
   because they will be referenced through pointers.
3. Push the address of each string plus a null pointer sentinel, on the
   stack, in right-to-left order. These are the elements of argv. The null pointer
   sentinel ensures that `argv[argc]` is a null pointer, as required by the C
   standard. The order ensures that `argv[0]` is at the lowest virtual address.
   Word-aligned accesses are faster than unaligned accesses, so for best
   performance round the stack pointer down to a multiple of 8 before the first
   push.
4. Point `%rsi` to `argv` (the address of `argv[0]`) and set `%rdi` to `argc`.
5. Finally, push a fake "return address": although the entry function will never
   return, its stack frame must have the same structure as any other.

The table below shows the state of the stack and the relevant registers right
before the beginning of the user program.
Note that the stack grows down.

|Address|Name|Data|Type|
| ----------| --------------| -----------| -----------|
|0x4747fffc|argv[3][...]|'bar\0'|char[4]|
|0x4747fff8|argv[2][...]|'foo\0'|char[4]|
|0x4747fff5|argv[1][...]|'-l\0'|char[3]|
|0x4747ffed|argv[0][...]|'/bin/ls\0'|char[8]|
|0x4747ffe8|word-align|0|uint8_t[]|
|0x4747ffe0|argv[4]|0|char *|
|0x4747ffd8|argv[3]|0x4747fffc|char *|
|0x4747ffd0|argv[2]|0x4747fff8|char *|
|0x4747ffc8|argv[1]|0x4747fff5|char *|
|0x4747ffc0|argv[0]|0x4747ffed|char *|
|0x4747ffb8|return address|0|void (*) ()|

> RDI: 4 | RSI: 0x4747ffc0

In this example, the stack pointer would be initialized to `0x4747ffb8`
As shown above, your code should start the stack at the `USER_STACK`, which is
defined in `include/threads/vaddr.h`.

You may find the non-standard `hex_dump()` function, declared in `<stdio.h>`,
useful for debugging your argument passing code.

#### Implement the argument passing.

Currently, `process_exec()` does not support passing arguments to new
processes. Implement this functionality, by extending `process_exec()` so
that instead of simply taking a program file name as its argument, it divides it
into words at spaces. The first word is the program name, the second word is the
first argument, and so on. That is, `process_exec("grep foo bar")` should run
grep passing two arguments foo and bar.

Within a command line, multiple spaces are equivalent to a single space, so that
`process_exec("grep foo bar")` is equivalent to our original example. You can
impose a reasonable limit on the length of the command line arguments. For
example, you could limit the arguments to those that will fit in a single page
(4 kB). (There is an unrelated limit of 128 bytes on command-line arguments that
the pintos utility can pass to the kernel.)

You can parse argument strings any way you like. If you're lost, look at
`strtok_r()`, prototyped in `include/lib/string.h` and implemented with thorough
comments in `lib/string.c`. You can find more about it by looking at the man
page (run `man strtok_r` at the prompt).
