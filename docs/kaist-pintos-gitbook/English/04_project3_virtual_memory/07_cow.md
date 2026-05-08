# 07_cow

## Copy-on-write (Extra)

**Implement copy-on-write mechanism in Pintos.**

Copy-on-write is a resource management technique that allows faster duplication
operation by using the same instance of the physical page. If some resource is
being used by multiple processes, each process should usually have its own copy
of the resource to make sure that no conflict occurs. If the resource is not
modified but just being read, however, there is no need to have multiple copies
in the physical memory.

For example, suppose a new process is created through `fork`. The child needs to
inherit its parent's resource by duplicating the data into its virtual address
space. Usually, adding contents to its virtual memory involves allocating a
physical page, writing the data into the frame, and adding a virtual->physical
mapping in its page table. These steps can be quite time-consuming.

With copy-on-write technique, however, we do not allocate new physical page for
the new copy of resources. This is because, technically, the content already exists in the
physical memory. Therefore, we only add the virtual->physical mapping in the
child process's page table, where the virtual address is now in the child's memory space.
Then, the parent and child are accessing the same
data in the same physical page. Yet, they are still isolated through separate
virtual address space, and only OS knows that they are referring to the same
frame. Only when one of the process tries to modify the content of the shared
resource, it will create a separate copy in a new physical page for itself.
Therefore, the actual copy operation is deferred to the first write.

That is, OS needs to be able to detect write attempts on copy-on-write pages. To
achieve this need, OS uses the "write-protect" mechanism. It is a simple
idea: Make a page fault on write accesses. This can be easily implemented with
the support of the memory management system, by just marking write-protected
page as unwritable at all.

You only need to **implement copy-on-write for **​**​`fork`​**. When a child process
inherits the resource from its parent process, the resource can be referencing
to the same physical data until the child tries to modify it. **All**​**write-protected pages are candidates for eviction.**

We only provide basic test cases for copy-on-write. You should consider all
the possible cases (Here is a small hint: you should implement sharing of
file-backed page). The grading of this extra project will be done with the
hidden test cases as well.
