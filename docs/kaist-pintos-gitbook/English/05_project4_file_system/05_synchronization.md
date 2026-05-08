# 05_synchronization

### Synchronization

The provided file system requires external synchronization, that is, callers must ensure that
only one thread can be running in the file system code at once. Your submission must adopt
a finer-grained synchronization strategy that does not require external synchronization. To
the extent possible, operations on independent entities should be independent, so that they
do not need to wait on each other.

Operations on different cache blocks must be independent. In particular, when I/O is
required on a particular block, operations on other blocks that do not require I/O should
proceed without having to wait for the I/O to complete.

Multiple processes must be able to access a single file at once. Multiple reads of a single
file must be able to complete without waiting for one another. When writing to a file does
not extend the file, multiple processes should also be able to write a single file at once. A
read of a file by one process when the file is being written by another process is allowed to
show that none, all, or part of the write has completed. (However, after the `write` system
call returns to its caller, all subsequent readers must see the change.) Similarly, when two
processes simultaneously write to the same part of a file, their data may be interleaved.

On the other hand, extending a file and writing data into the new section must be
atomic. Suppose processes A and B both have a given file open and both are positioned at
end-of-file. If A reads and B writes the file at the same time, A may read all, part, or none
of what B writes. However, A may not read data other than what B writes, e.g. if B's data
is all nonzero bytes, A is not allowed to see any zeros.

Operations on different directories should take place concurrently. Operations on the
same directory may wait for one another.

Keep in mind that only data shared by multiple threads needs to be synchronized. In
the base file system, `struct file` and `struct dir` are accessed only by a single thread.
