# 02_vm_management

## Memory Management

In order to support your virtual memory system, you need to effectively
manage virtual pages and physical frames. This means that you have to keep track
of which (virtual or physical) memory regions are being used, for what purpose,
by whom, and so on. You will first deal with supplemental page table, then
physical frames. Note that, for your understanding, we use the term "page" for
virtual page, and the term "frame" for a physical page.

### Page Structure and Operations

#### struct page

A `page` defined in `include/vm/vm.h` is a structure that represents a page in
virtual memory. It stores all the necessary data that we need to know about the
page. Currently, the structure looks like the following in the template:

```c
struct page {
  const struct page_operations *operations;
  void *va;              /* Address in terms of user space */
  struct frame *frame;   /* Back reference for frame */

  union {
    struct uninit_page uninit;
    struct anon_page anon;
    struct file_page file;
#ifdef EFILESYS
    struct page_cache page_cache;
#endif
  };
};
```

It has page operations (read below), virtual address, and physical frame.
Additionally, it has a union field. A union is a special data type that
allows us to store different types of data in a memory region.
There are multiple members in the union, but only one member can contain a value
at a time. This means that a page in our system can be a uninit_page, anon_page,
file_page, or page_cache.
For example, if a page is an anonymous page (See [Anonymous Page](03_anon.md)),
then the page struct will have the field `struct anon_page anon` as one of its
members. `anon_page` will contain all the necessary information we need to keep
for an anonymous page.

#### Page Operations

As described above and defined in `include/vm/vm.h`,
a page at can be `VM_UNINIT`, `VM_ANON`, or `VM_FILE`.
For a page, there are several actions to be taken, such as swapping in,
swapping out, and destroying the page. For each type of page,
the required steps and tasks differ for these actions. In other words, a different `destroy`
function needs to be invoked for a `VM_ANON` page and `VM_FILE` page. One way is
use a switch-case syntax in each function to handle each case. We introduce a
"class inheritance" concept of Object-Oriented-Programming to deal it.
Indeed, there is no "class" nor "inheritance" in C programming language, we
utilizes [function pointers](https://www.geeksforgeeks.org/function-pointer-in-c/)
to realize the concepts, in a similar way in real operating system codes like Linux.

A function pointer is a pointer, just like any other pointers you've learned
sofar, that points to a function, or an executable code within the memory.
Function pointers are useful because they provide a simple way to call a
specific function to execute based on run-time values without any checking.
In our case, simply calling `destroy (page)` is sufficient at code-level, and the
compiler will choose the appropriate `destroy` routine depending on the page type
by calling the right function pointer.

A structure for page operations `struct page_operations` is defined in
`include/vm/vm.h`. Consider this structure as a table of functions, containing 3
function pointers.

```c
struct page_operations {
  bool (*swap_in) (struct page *, void *);
  bool (*swap_out) (struct page *);
  void (*destroy) (struct page *);
  enum vm_type type;
};
```

Now, let's see where we can find a page_operation structure. If you look into a
page structure `struct page` in `include/vm/vm.h`, you see that it has a field
named `operations`. Now, go to `vm/file.c`, you will see the page_operations
structure `file_ops` declared before the function prototypes. This is the table
of function pointers for file-backed pages. `.destroy` field has value
`file_backed_destroy`, which is a function that destroys the page and is defined in
the same file.

Let's try to understand how `file_backed_destroy` is invoked with the function
pointer interface. Suppose `vm_dealloc_page (page)` (in `vm/vm.c`) is called,
and this page happens to be a file-backed page (`VM_FILE`). Inside the function,
it invokes `destroy (page)`. `destroy (page)` is defined with a macro in
`include/vm/vm.h` like the following:

```c
#define destroy(page) if ((page)->operations->destroy) (page)->operations->destroy (page)
```

This tells us that calling `destroy` function actually invokes
`(page)->operations->destroy (page)`, which is the `destroy` function retrieved
from the page structure. Because the page is a `VM_FILE` page, its `.destroy`
field points to `file_backed_destory`. As a result, destroy routine for file-backed
page is performed.

### Implement Supplemental Page Table

At this point, your Pintos has a page table (`pml4`) to manage virtual and
physical mappings of memory. However, this is not enough. As discussion in the
previous section, you will also need a supplementary page table to hold additional
information about each page in order to handle page fault and resource
management. Therefore, we suggest you to implement some basic functionalities
for the supplemental page table as the first task of project 3.

**Implement supplemental page table management functions in **​**​`vm/vm.c`​**​ **.**

You will first have to decide how you will design the supplemental page table in
your Pintos. After design your own supplemental page table, implements below
three functions with respect to your design.

---

```c
void supplemental_page_table_init (struct supplemental_page_table *spt);
```

> Initializes the supplemental page table. You may choose the data structure to
> use for the supplemental page table. The function is called when a new process
> starts (in `initd` of `userprog/process.c`) and when a process is being forked
> (in `__do_fork` of `userprog/process.c`).

---

```c
struct page *spt_find_page (struct supplemental_page_table *spt, void *va);
```

> Find `struct page` that corresponds to va from the given supplemental page
> table. If fail, return NULL.

---

```c
bool spt_insert_page (struct supplemental_page_table *spt, struct page *page);
```

> Insert `struct page` into the given supplemental page table. This function
> should checks that the virtual address does not exist in the given
> supplemental page table.

### Frame Management

From now, all the pages are not just holding the meta-data for the memory when
it was constructed. Therefore, we need a different scheme to manage the physical
memory. In `include/vm/vm.h`, there exists `struct frame` which represents a
physical memory. Currently, the structure looks like this:

```c
/* The representation of "frame" */
struct frame {
  void *kva;
  struct page *page;
};
```

It only has two fields - `kva` which is the
kernel virtual address and `page` which is a page structure.
You are allowed to add more members as you implement a frame management interface.

**Implement **​**​`vm_get_frame`​**​ **, **​**​`vm_claim_page`​**​ ** and **​**​`vm_do_claim_page`​**​ ** in**​**​`vm/vm.c`​**​ **.**

---

```c
static struct frame *vm_get_frame (void);
```

> Gets a new physical page from the user pool by calling `palloc_get_page`.
> When successfully got a page from the user pool, also allocates a frame,
> initialize its members, and returns it.
> After you implement `vm_get_frame`, you have to allocate all user space pages
> (PALLOC_USER) through this function. You don't need to handle swap out for now
> in case of page allocation failure. Just mark those case with `PANIC ("todo")`
> for now.

---

```c
bool vm_do_claim_page (struct page *page);
```

> Claims, meaning allocate a physical frame, a page. You first get a frame by
> calling `vm_get_frame` (which is already done for you in the template).
> Then, you need to set up the MMU. In other words, add the mapping from the
> virtual address to the physical address in the page table. The return value
> should indicate whether the operation was successful or not.

---

```c
bool vm_claim_page (void *va);
```

> Claims the page to allocate `va`. You will first need to get a page and then
> calls vm_do_claim_page with the page.
