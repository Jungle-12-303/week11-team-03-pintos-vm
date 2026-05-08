# 04_virtual_address

# Virtual Addresses

A 64-bit virtual addresses are structured as follows:

```
63          48 47            39 38            30 29            21 20         12 11         0
+-------------+----------------+----------------+----------------+-------------+------------+
| Sign Extend |    Page-Map    | Page-Directory | Page-directory |  Page-Table |  Physical  |
|             | Level-4 Offset |    Pointer     |     Offset     |   Offset    |   Offset   |
+-------------+----------------+----------------+----------------+-------------+------------+
              |                |                |                |             |            |
              +------- 9 ------+------- 9 ------+------- 9 ------+----- 9 -----+---- 12 ----+
                                          Virtual Address
```

Header `include/threads/vaddr.h`, and `include/threads/mmu.h` define these
functions and macros for working with virtual addresses:

---

```c
#define PGSHIFT { /* Omit details */ }
#define PGBITS { /* Omit details */ }
```

> The bit index (0) and number of bits (12) of the offset part of a virtual
> address, respectively.

---

```c
#define PGMASK { /* Omit details */ }
```

> A bit mask with the bits in the page offset set to 1, the rest set to 0
> (0xfff).

---

```c
#define PGSIZE { /* Omit details */ }
```

> The page size in bytes (4,096).

---

```c
#define pg_ofs(va) { /* Omit details */ }
```

> Extracts and returns the page offset in virtual address va.

---

```c
#define pg_no(va) { /* Omit details */ }
```

> Extracts and returns the page number in virtual address va.

---

```c
#define pg_round_down(va) { /* Omit details */ }
```

> Returns the start of the virtual page that va points within, that is, va
> with the page offset set to 0.

---

```c
#define pg_round_up(va) { /* Omit details */ }
```

> Returns va rounded up to the nearest page boundary.

Virtual memory in Pintos is divided into two regions: user virtual memory and
kernel virtual memory (see [Virtual Memory Layout](../03_project2_user_programs/01_introduction.md)).

The boundary between them is `KERN_BASE`:

---

```c
#define KERN_BASE { /* Omit details */ }
```

> Base address of kernel virtual memory. It defaults to 0x8004000000.
> User virtual memory ranges from virtual address 0 up to `KERN_BASE`.
> Kernel virtual memory occupies the rest of the virtual address space.

---

```c
#define is_user_vaddr(vaddr) { /* Omit details */ }
#define is_kernel_vaddr(vaddr) { /* Omit details */ }
```

> Returns true if va is a user or kernel virtual address, respectively, false
> otherwise.

---

The x86-64 doesn't provide any way to directly access memory given
a physical address. This ability is often necessary in an operating system
kernel, so Pintos works around it by mapping kernel virtual memory
one-to-one to physical memory. That is, virtual address > `KERN_BASE`
accesses physical address 0, virtual address `KERN_BASE + 0x1234` accesses
physical address 0x1234, and so on up to the size of the machine's physical
memory. Thus, adding `KERN_BASE` to a physical address obtains a kernel
virtual address that accesses that address; conversely, subtracting
`KERN_BASE` from a kernel virtual address obtains the corresponding physical
address.

Header `include/threads/vaddr.h` provides a pair of functions to do these
translations:

---

```c
#define ptov(paddr) { /* Omit details */ }
```

> Returns the kernel virtual address corresponding to physical address pa, which
> should be between 0 and the number of bytes of physical memory.

---

```c
#define vtop(vaddr) { /* Omit details */ }
```

> Returns the physical address corresponding to va, which must be a kernel
> virtual address.

Header `include/threads/mmu.h` provides operations on page table:

---

```c
#define is_user_pte(pte) { /* Omit details */ }
#define is_kern_pte(pte) { /* Omit details */ }
```

> Query whether the page table entry (PTE) is owned by user or kernel,
> respectively.

---

```c
#define is_writable(pte) { /* Omit details */ }
```

> Query whether virtual address pointed by the page table entry (PTE) is
> wriatable or not.

---

```c
typedef bool pte_for_each_func (uint64_t *pte, void *va, void *aux);
bool pml4_for_each (uint64_t *pml4, pte_for_each_func *func, void *aux);
```

> For each valid entry under PML4, apply FUNC with auxillary value AUX.
> VA represents the virtual address of the entry. If pte_for_each_func returns
> false, stop iteration and return false.

Below shows an example `func` that could be fed to `pml4_for_each`:

```c
static bool
stat_page (uint64_t *pte, void *va,  void *aux) {
        if (is_user_vaddr (va))
                printf ("user page: %llx\n", va);
        if (is_writable (va))
                printf ("writable page: %llx\n", va);
        return true;
}
```
