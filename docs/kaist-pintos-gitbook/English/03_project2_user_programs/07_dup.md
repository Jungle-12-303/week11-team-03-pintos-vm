# 07_dup

## Extend File Descriptor (Extra)

**Make your pintos to support closing of stdin, stdout and dup2 system call in**​**linux**

The closing the fd of `stdin` and `stdout` is forbidden in current
implementation of pintos. In this extra point requirements, you first allow user
to close `stdin` and `stdout` as same as the linux. That is, the process should
never read the inputs if you close `stdin`, and the process should never print
out anything if you close `stdout`.

Next, implement the `dup2` system call.

---

```c
int dup2(int oldfd, int newfd);
```

> The `dup2()` system call creates a copy of the file descriptor `oldfd` with
> the file descriptor number specified in `newfd`, and returns `newfd` on
> success. If the file descriptor `newfd` was previously open, it is silently
> closed before being reused.
>
> Note the following points:
>
> - If `oldfd` is not a valid file descriptor, then the call fails
>   (returns `-1`), and `newfd` is not closed.
> - If `oldfd` is a valid file descriptor, and `newfd` has the same value as
>   `oldfd`, then `dup2()` does nothing, and returns `newfd`.
>
> After a successful return from this system call, the old and new file
> descriptors may be used interchangeably. Although they are **different file**​**descriptors**, they **refer to the same open file description** and thus share
> *file offset* and *status flags*; for example, if the file offset is modified
> by using `seek` on one of the descriptors, the offset is also changed for the
> other.

Note that `dup`ed file descriptors must preserve their semantic after the
forking.

**YOU SHOULD PASS ALL TESTCASES FOR EXTRA TO GET THE CREDIT.**​**TAKE ALL OR NOTHING.**
