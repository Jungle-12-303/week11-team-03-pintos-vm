# 03_anon

## Anonymous Page

In this part of this project, you will implement the non-disk based image
called *anonymous page*.

An anonymous mapping has no backing file or device. It is anonymous
because it does not have any named file source (unlike file-backed pages).
Anonymous pages are used in executable, such as for stack and heap.

There is a struct to describe an anonymous page - `anon_page` in
`include/vm/anon.h`. It is currently empty, but you may add members to store
necessary information or state of an anonymous page as you implement. Also, see
the `struct page` in `include/vm/page.h`, which contains generic information
of a page. Note, for an anonymous page, `struct anon_page anon` is included in
its page structure.

### Page Initialization with Lazy Loading

Lazy loading is a design where the loading of memory is deferred until the point
at which it is needed. A page is allocated, meaning there is page struct
corresponding to it, but there is no dedicated physical frame, and the actual
content of the page is not yet loaded. The contents will be loaded only at which
it is truly needed, which is signaled by a page fault.

Since we have three page types, the initialization routine is different for each
page. Though will be explained again in the sections below, here we provide a
high-level view of the page initialization flow. First,
`vm_alloc_page_with_initializer` is invoked when the kernel receives a new page
request. The initializer will initialize a new page by allocating a page
structure and setting appropriate initializer depending on its page type, and
return the control back to the user program. As the user program executes, at a
point, a page fault occurs because the program is trying to access a page which
it believes to possess but the page has no contents yet. During the fault
handling procedure, `uninit_initialize` is invoked and calls the initializer you
set earlier. The initializer will be `anon_initializer` for anonymous pages and
`file_backed_initializer` for file-backed pages.

A page can have a life cycle of
initialize->(page_fault->lazy-load->swap-in>swap-out->...)->destroy.
For each transition of the life cycle, the required procedure differs
depending on the page type (or `VM_TYPE`), and the previous paragraph
was an example for the initialization. In this project, you will implement
these transition processes for each page type.

### Lazy Loading for Executable

In lazy loading, when a process starts it execution, only
the memory parts that are immediately needed are loaded onto the main memory.
This can reduce the overhead compared to eager loading, which loads all binary
image into the memory at once.

To support the lazy loading, we introduce a page type called `VM_UNINIT` in
`include/vm/vm.h`. All pages are initially created as `VM_UNINIT` pages. We also
provide a page structure for uninitialized pages - `struct uninit_page` in
`include/vm/uninit.h`. The functions for creating, initializing, and destroying
uninitialized pages can be found in `include/vm/uninit.c`. You will have to
complete these functions later.

On page fault, the page fault handler (`page_fault` in `userprog/exception.c`)
transfers control to `vm_try_handle_fault` in `vm/vm.c`, which first checks if
it is a valid page fault. By *valid*, we mean the fault that accesses invalid.
If it is a *bogus* fault, you load some contents into the page and return
control to the user program.

There are three cases of bogus page fault: lazy-loaded, swaped-out page, and
write-protected page (See [Copy-on-Write (Extra)](07_cow.md)). For now, just
consider the first case, lazy-loaded page. If it is a page fault for lazy
loading, the kernel calls one of the initializers you previously set in
`vm_alloc_page_with_initializer` to lazy load the segment. You will have to
implement `lazy_load_segment` in `userprog/process.c`.

**Implement **​**​`vm_alloc_page_with_initializer()`​**​ **.** 
You should fetch an appropriate initializer according to the passed vm_type and
call `uninit_new` with it.

---

```c
bool vm_alloc_page_with_initializer (enum vm_type type, void *va,
        bool writable, vm_initializer *init, void *aux);
```

> Create an uninitialized page with the given type. The swap_in handler of uninit
> page automatically initializes the page according to the type, and calls
> INIT with given AUX. Once you have the page struct, insert the page into
> the process's supplementary page table. Using `VM_TYPE` macro defined in
> `vm.h` can be handy.

The page fault handler follows its call chain, and finally reaches
`uninit_intialize` when it calls swap_in. We gives the complete implementation
for it. Although, you may need to modify the `uninit_initialize` according to
your design.

---

```c
static bool uninit_initialize (struct page *page, void *kva);
```

> Initializes the page on the first fault. The template code first fetches
> `vm_initializer` and `aux` and calls the corresponding page_initializer
> through a function pointer. You may need to modify the function depending
> on your design.

You may modify `vm_anon_init` and `anon_initializer` in `vm/anon.c` according
to your needs.

---

```c
void vm_anon_init (void);
```

> Initialize for anonymous page subsystem. In this function, you can setup
> anything related to the anonymous page.

---

```c
bool anon_initializer (struct page *page,enum vm_type type, void *kva);
```

> The function first sets up the handlers for the anonymous page in
> `page->operations`. You might need to update some information in `anon_page`,
> which is currently an empty struct. This function is used as initializer for
> anonymous pages (i.e. `VM_ANON`).

**Implement **​**​`load_segment`​**​ ** and **​**​`lazy_load_segment`​**​ ** in **​**​`userprog/process.c`​**​ **.** 
Implement segment loading from executables. All of these pages should be
loaded lazily, that is, only as the kernel intercepts page faults for them.

You'll need to modify the core of the program loader, which is the loop in
`load_segment` of `userprog/process.c`. Each time around the loop, it makes a
call to `vm_alloc_page_with_initializer` to create a pending page object.
When a page fault occurs, this is when the segment is actually loaded from the
file.

---

```c
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
        uint32_t read_bytes, uint32_t zero_bytes, bool writable);
```

> Current code calculates the number of bytes to read from a file and the number
> of bytes to fill with zeros within the main loop. Then, it calls
> `vm_alloc_page_with_initializer` to create a pending object. You need to set
> up the auxiliary values as `aux` argument that you will provide to
> `vm_alloc_page_with_initializer`. You may want to create a structure that
> contains necessary information for the loading of binary.

---

```c
static bool lazy_load_segment (struct page *page, void *aux);
```

> You may have noticed that `lazy_load_segment` is supplied as the fourth
> argument of `vm_alloc_page_with_initializer` in `load_segment`. This function
> is the initializer for executable's pages and is invoked in times of page
> faults. It receives a page struct and `aux` as arguments. `aux` is the
> information you set up in `load_segment`. Using this information, you have to
> find the file to read the segment from and eventually read the segment into
> memory.

**You should adjust the **​**​`setup_stack`​**​ ** in **​**​`userprog/process.c`​**​ ** to fit stack**​**allocation into the new memory management system.**  The first stack page need not be
allocated lazily. You can allocate and initialize it with the command line
arguments at load time, with no need to wait for it to be faulted in. You might
need to provide the way to identify the stack. You can use the auxillary markers
in `vm_type` of `vm/vm.h` (e.g. `VM_MARKER_0`) to mark the page.

Finally, modify `vm_try_handle_fault` function to resolve the page struct
corresponding to the faulted address by consulting to the supplemental page
table through `spt_find_page`.

After implementing all the requirements, all of the tests in project 2 except
fork should be passed.

### Supplemental Page Table - Revisit

Now, we revisit the supplemental page table interface to support copy and clean up operations.
These operations are needed when creating (more specifically generating a child process)
or destroying a process. The details are elaborated below. The reason we are revisiting the
supplemental page table at this point is because you may want to use some of the
initializing functions you implemented from above.

**Implement **​**​`supplemental_page_table_copy`​**​ ** and **​**​`supplemental_page_table_kill`​**​ ** in**​**​`vm/vm.c`​**​ **.**

---

```c
bool supplemental_page_table_copy (struct supplemental_page_table *dst,
    struct supplemental_page_table *src);
```

> Copies the supplemental page table from src to dst. This is used when a child
> needs to inherit the execution context of its parent (i.e. `fork()`). Iterate
> through each page in the src's supplemental page table and make a exact copy
> of the entry in the dst's supplemental page table. You will need to allocate
> uninit page and claim them immediately.

---

```c
void supplemental_page_table_kill (struct supplemental_page_table *spt);
```

> Frees all the resources that were held by a supplemental page table.
> This function is called when a process exits (`process_exit()` in
> `userprog/process.c`). You need to iterate through the page entries and call
> `destroy(page)` for the pages in the table. You do not need to worry about the
> actual page table (pml4) and the physical memory (palloc-ed memory) in this
> function; the caller cleans them after the supplemental page table is cleaned
> up.

### Page Cleanup

**Implement **​**​`uninit_destroy`​**​ ** in **​**​`vm/uninit.c`​**​ ** and **​**​`anon_destroy`​**​ ** in**​**​`vm/anon.c`​**​ **.**  This is handler for `destroy` operation on uninitialized page.
Even though uninitialized pages are transmuted to the other page objects, there
still can be uninit page when the process exits.

---

```c
static void uninit_destroy (struct page *page);
```

> Frees the resource that was held by page struct. You might
> want to check the vm type of the page and handle accordingly.

For now, you can only handle the anonymous pages. You will later revisit this
function to clean up the file-backed pages.

---

```c
static void anon_destroy (struct page *page);
```

> Frees the resource that was held by the anonymous page. You do not need to
> explicitly free the page struct, the caller should do it.

Now all of the tests in project 2 should be passed.
