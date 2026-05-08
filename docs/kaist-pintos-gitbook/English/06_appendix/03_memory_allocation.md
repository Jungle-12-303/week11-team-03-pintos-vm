# 03_memory_allocation

# Memory Allocation

Pintos contains two memory allocators, one that allocates memory in units of a
page, and one that can allocate blocks of any size.

## Page Allocator

The page allocator declared in `include/threads/palloc.h` allocates memory
in units of a page. It is most often used to allocate memory one page at a time,
but it can also allocate multiple contiguous pages at once.

The page allocator divides the memory it allocates into two pools, called the
kernel and user pools. By default, each pool gets half of system memory above
1MB, but the division can be changed with the **ul** kernel command line option.
An allocation request draws from one pool or the other. If one pool becomes
empty, the other may still have free pages. The user pool should be used for
allocating memory for user processes and the kernel pool for all other
allocations. This will only become important starting with project 3. Until
then, all allocations should be made from the kernel pool.

Each pool's usage is tracked with a bitmap, one bit per page in the pool. A
request to allocate n pages scans the bitmap for n consecutive bits set to
false, indicating that those pages are free, and then sets those bits to true to
mark them as used. This is a "first fit" allocation strategy.

The page allocator is subject to fragmentation. That is, it may not be possible
to allocate n contiguous pages even though n or more pages are free, because
the free pages are separated by used pages. In fact, in pathological cases it
may be impossible to allocate 2 contiguous pages even though half of the pool's
pages are free. Single-page requests can't fail due to fragmentation, so
requests for multiple contiguous pages should be limited as much as possible.

Pages may not be allocated from interrupt context, but they may be freed.

When a page is freed, all of its bytes are cleared to **0xcc**, as a debugging
aid (see [Debugging Tips](06_debugging_tools.md#tips)).

Page allocator types and functions are described below.

---

```c
void *palloc_get_page (enum palloc_flags flags)
void *palloc_get_multiple (enum palloc_flags flags, size_t page_cnt)
```

> Obtains and returns one page, or *page_cnt* contiguous pages, respectively.
> Returns a null pointer if the pages cannot be allocated.
>
> The *flags* argument may be any combination of the following flags:
>
> - `PAL_ASSERT`
>
>   If the pages cannot be allocated, panic the kernel. This is only appropriate
>   during kernel initialization. User processes should never be permitted to
>   panic the kernel.
> - `PAL_ZERO`
>
>   Zero all the bytes in the allocated pages before returning them.
>   If not set, the contents of newly allocated pages are unpredictable.
> - `PAL_USER`
>
>   Obtain the pages from the user pool. If not set, pages are allocated from
>   the kernel pool.

---

```c
void palloc_free_page (void *page)
void palloc_free_multiple (void *pages, size_t page_cnt)
```

> Frees one page, or page cnt contiguous pages, respectively, starting at
> *pages*. All of the pages must have been obtained using `palloc_get_page()`
> or `palloc_get_multiple()`.

## Block Allocator

The block allocator, declared in `threads/malloc.h`, can allocate blocks of any
size. It is layered on top of the page allocator described in the previous
section. Blocks returned by the block allocator are obtained from the kernel
pool.

The block allocator uses two different strategies for allocating memory. The
first strategy applies to blocks that are 1 kB or smaller
(one-fourth of the page size). These allocations are rounded up to the nearest
power of 2, or 16 bytes, whichever is larger. Then they are grouped into a page
used only for allocations of that size.

The second strategy applies to blocks larger than 1 kB. These allocations
(plus a small amount of overhead) are rounded up to the nearest page in size,
and then the block allocator requests that number of contiguous pages from the
page allocator.

In either case, the difference between the allocation requested size and the
actual block size is wasted. A real operating system would carefully tune its
allocator to minimize this waste, but this is unimportant in an instructional
system like Pintos.

As long as a page can be obtained from the page allocator, small allocations
always succeed. Most small allocations do not require a new page from the page
allocator at all, because they are satisfied using part of a page already
allocated. However, large allocations always require calling into the page
allocator, and any allocation that needs more than one contiguous page can fail
due to fragmentation, as already discussed in the previous section. Thus, you
should minimize the number of large allocations in your code, especially those
over approximately 4 kB each.

When a block is freed, all of its bytes are cleared to **0xcc**, as a debugging
aid (see [Debugging Tips](06_debugging_tools.md#tips)).

The block allocator may not be called from interrupt context.

The block allocator functions are described below. Their interfaces are the same
as the standard C library functions of the same names.

---

```c
void *malloc (size_t size)
```

> Obtains and returns a new block, from the kernel pool, at least *size* bytes long.
> Returns a null pointer if size is zero or if memory is not available.

---

```c
void *calloc (size_t a, size_t b)
```

> Obtains a returns a new block, from the kernel pool, at least a * b bytes
> long. The block's contents will be cleared to zeros. Returns a null pointer if
> a or b is zero or if insufficient memory is available.

---

```c
void *realloc (void *block, size_t new_size)
```

> Attempts to resize *block* to *new_size* bytes, possibly moving it in the
> process. If successful, returns the new block, in which case the old block
> must no longer be accessed. On failure, returns a null pointer, and the old
> block remains valid. A call with block null is equivalent to `malloc()`. A
> call with new size zero is equivalent to `free()`.

---

```c
void free (void *block)
```

> Frees *block*, which must have been previously returned by `malloc()`,
> `calloc()`, or `realloc()` (and not yet freed).
