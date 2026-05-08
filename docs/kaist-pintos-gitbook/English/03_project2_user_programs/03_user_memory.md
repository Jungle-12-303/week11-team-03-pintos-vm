# 03_user_memory

### User Memory Access

**Implement user memory access**

To implement syscalls, you need to provide ways to read and write
data in user virtual address space. You don't need this ability when getting the
arguments. However, when you read the data from the pointer provided as the
system call's argument, you should proxied through this functionallity.
This can be a bit tricky: what if the user provides an invalid pointer, a
pointer into kernel memory, or a block partially in one of those regions? You
should handle these cases by terminating the user process.
