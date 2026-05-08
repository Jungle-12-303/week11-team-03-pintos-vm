# 05_memory_mapped_files

### Memory Mapped Files

In this section, you will implement memory-mapped pages. Unlike anonymous pages,
memory-mapped pages are file-backed mappings. The contents in the page mirror
data in some existing file. If a page fault occurs, a physical frame is
immediately allocated and the contents are copied into the memory from the file.
When memory-mapped pages are unmapped or swapped out, any change in the content
is reflected in the file.

#### `mmap` and `munmap` System Call

**Implement **​**​`mmap`​**​ ** and **​**​`munmap`​**, which are the two system calls for memory
mapped files. Your VM system must load pages lazily in mmap regions and use the
mmaped file itself as a backing store for the mapping. You should implement and
use `do_mmap` and `do_munmap` defined in `vm/file.c` to implement these two
system calls.

---

```c
void *mmap (void *addr, size_t length, int writable, int fd, off_t offset);
```

> Maps `length` bytes the file open as `fd` starting from `offset` byte into the
> process's virtual address space at `addr`. The entire file is mapped into
> consecutive virtual pages starting at `addr`. If the length of the file is not
> a multiple of PGSIZE, then some bytes in the final mapped page "stick out"
> beyond the end of the file. Set these bytes to zero when the page is faulted
> in, and discard them when the page is written back to disk. If successful,
> this function returns the virtual address where the file is mapped. On
> failure, it must return NULL which is not a valid address to map a file.

A call to `mmap` may fail if the file opened as fd has a length of zero bytes.
It must fail if addr is not page-aligned or if the range of pages mapped
overlaps any existing set of mapped pages, including the stack or pages mapped
at executable load time. In Linux, if `addr` is NULL, the kernel finds an
appropriate address at which to create the mapping. For simplicity, you can just
attempt to mmap at the given `addr`. Therefore, if `addr` is 0, it must fail,
because some Pintos code assumes virtual page 0 is not mapped. Your mmap should
also fail when `length` is zero. Finally, the file descriptors representing
console input and output are not mappable.

Memory-mapped pages should be also allocated in a lazy manner just like
anonymous pages. You can use `vm_alloc_page_with_initializer` or `vm_alloc_page`
to make a page object.

---

```c
void munmap (void *addr);
```

> Unmaps the mapping for the specified address range `addr`, which must be the
> virtual address returned by a previous call to mmap by the same process that
> has not yet been unmapped.

All mappings are implicitly unmapped when a process exits, whether via `exit` or
by any other means. When a mapping is unmapped, whether implicitly or
explicitly, all pages written to by the process are written back to the file,
and pages not written must not be. The pages are then removed from the process's
list of virtual pages.

Closing or removing a file does not unmap any of its mappings. Once created, a
mapping is valid until `munmap` is called or the process exits, following the
Unix convention. See [Removing an Open File](../03_project2_user_programs/08_faq.md) for more
information. You should use the `file_reopen` function to obtain a separate and
independent reference to the file for each of its mappings.

If two or more processes map the same file, there is no requirement that they
see consistent data. Unix handles this by making the two mappings share the same
physical page, and the mmap system call also has an argument allowing the client
to specify whether the page is shared or private (i.e. copy-on-write).

You may want to modify `vm_file_init` and `vm_file_initializer` in `vm/vm.c`
according to your needs.

---

```c
void vm_file_init (void);
```

> Initializes the file-backed page subsystem. In this function, you can setup
> anything related to the file backed page.

---

```c
bool file_backed_initializer (struct page *page, enum vm_type type, void *kva);
```

> Initializes a file-backed page. The function first sets up the handlers for
> file-backed pages in `page->operations`. You may want to update some
> information on the page struct, such as the file that is backing the memory.

---

```c
static void file_backed_destroy (struct page *page);
```

> Destroys the file backed page by closing the associated file. If the content
> is dirty, make sure you write back the changes into the file. You do not need
> to free the page struct in this function. The caller of `file_backed_destroy`
> should handle it.
