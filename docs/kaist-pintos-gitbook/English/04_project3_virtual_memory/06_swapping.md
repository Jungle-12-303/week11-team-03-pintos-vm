# 06_swapping

## Swap In/Out

Memory swapping is a memory reclamation technique to maximize the usage of
physical memory. When frames of the main memory is allocated, the system cannot
handle any more memory allocation requests from user programs. One solution is
to swap out memory frames that are not being currently used to disk. This frees
some memory resources and makes available to other applications.

Swapping is done by the operating system. When the system detects that it has
run out of memory but receives a memory allocation request, it chooses a page to
evict out to swap disk. Then, the exact state of the memory frame is copied to
the disk. When a process tries to access a swapped out page, OS recovers the
page by bringing the exact content back to the memory.

The page chosen for eviction may be an anonymous page or a file-backed page.
In this section, you will handle each case.

All the swapping operations are not called explicitly, but as function pointers.
They are members of `struct page_operations file_ops`, which will be registered
as operations for each page's initializer.

### Anonymous Page

**Modify **​**​`vm_anon_init`​**​ ** and **​**​`anon_initializer`​**​ ** in **​**​`vm/anon.c`​**​ **.**  Anonymous page
doesn't have any backing storage for it. To support the swapping of anonymous
page, we provide the temporal backing storage called swap disk. You will utilize
the swap disk to implement the swap for anonymous pages.

---

```c
void vm_anon_init (void);
```

> In this function, you need to set up the swap disk. You will also need a data
> structure to manage free and used areas in the swap disk. The swap area will
> be also managed at the granularity of PGSIZE (4096 bytes)

---

```c
bool anon_initializer (struct page *page, enum vm_type type, void *kva);
```

> This is the initializer for the anonymous page. You will need to add some
> information to the `anon_page` to support the swapping.

Now, implement `anon_swap_in` and `anon_swap_out` in `vm/anon.c` to support
swapping for anonymous pages. Since a page needs to swapped out to be swapped
in, you may want to implement `anon_swap_out` before implementing
`anon_swap_in`. You need to move the data contents to the swap disk, and bring
it back to memory safely.

---

```c
static bool anon_swap_in (struct page *page, void *kva);
```

> Swaps in an anonymous page from the swap disk by reading the data contents
> from the disk to memory. The location of the data is the swap disk should have
> been saved in the page struct when the page was swapped out. Remember to
> update the swap table (See [Managing the Swap Table](01_introduction.md)).

---

```c
static bool anon_swap_out (struct page *page);
```

> Swaps out an anonymous page to the swap disk by copying the contents
> from the memory to the disk. First, find a free swap slot in the disk using
> the swap table, then copy the page of data into the slot. The location of the
> data should be saved in the page struct. If there is no more free slot in the
> disk, you can panic the kernel.

### File-Mapped Page

Since the contents of a file-backed page comes from a file, the mmaped file
should be used as a backing store. That is, evicting a file-backed page writes
it back to the file it was mapped from. Implement `file_backed_swap_in`,
`file_backed_swap_out` in `vm/file.c`. You might modify `file_backed_init` and
`file_initializer` according to your design.

---

```c
static bool file_backed_swap_in (struct page *page, void *kva);
```

> Swaps in a page at `kva` by reading the contents in from the file. You need to
> synchronize with the file system.

---

```c
static bool file_backed_swap_out (struct page *page);
```

> Swaps out a page by writing the contents back to the file. You may want to
> first check if the page is dirty. If it is not dirty, you do not have to
> modify the contents in the file. After you swap out the page, remember to turn
> off the dirty bit for the page.
