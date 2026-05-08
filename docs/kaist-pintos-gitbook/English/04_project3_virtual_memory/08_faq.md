# 08_faq

# FAQ

### Do we need a working Project 2 to implement Project 3?

Yes.

### How do we resume a process after we have handled a page fault?

Returning from `page_fault()` resumes the current user process. It will then retry the
instruction to which the instruction pointer points.
Why do user processes sometimes fault above the stack pointer?
You might notice that, in the stack growth tests, the user program faults on
an address that is above the user program’s current stack pointer.

### Does the virtual memory system need to support data segment growth?

No. The size of the data segment is determined by the linker. We still have
no dynamic allocation in Pintos (although it is possible to “fake” it at the user
level by using memory-mapped files). Supporting data segment growth should
add little additional complexity to a well-designed system.

### Why should I use `PAL_USER` for allocating page frames?

Passing `PAL_USER` to `palloc_get_page()` causes it to allocate memory from
the user pool, instead of the main kernel pool. Running out of pages in the user
pool just causes user programs to page, but running out of pages in the kernel
pool will cause many failures because so many kernel functions need to obtain
memory. You can layer some other allocator on top of `palloc_get_page()` if
you like, but it should be the underlying mechanism.
Also, you can use the `-ul` kernel command-line option to limit the size of the
user pool, which makes it easy to test your VM implementation with various
user memory sizes.
