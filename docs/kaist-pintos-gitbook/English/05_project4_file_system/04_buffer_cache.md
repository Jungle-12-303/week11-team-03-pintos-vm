# 04_buffer_cache

### Buffer Cache (Extra Credit)

Modify the file system to keep a cache of file blocks. When a request is made to read or
write a block, check to see if it is in the cache, and if so, use the cached data without
going to disk. Otherwise, fetch the block from disk into the cache, evicting an older entry
if necessary. You are limited to a cache no greater than 64 sectors in size.

You must implement a cache replacement algorithm that is at least as good as the
"clock" algorithm. We encourage you to account for the generally greater value
of metadata compared to data. Experiment to see what combination of accessed,
dirty, and other information results in the best performance, as measured by the
number of disk accesses.

You can keep a cached copy of the free map permanently in memory if you like. It
doesn't have to count against the cache size.

The provided inode code uses a "bounce buffer" allocated with `malloc()` to translate
the disk's sector-by-sector interface into the system call interface's byte-by-byte interface.
You should get rid of these bounce buffers. Instead, **copy data into and out of sectors in**​**the buffer cache directly.**

Your cache should be write-behind, that is, keep dirty blocks in the cache, instead of
immediately writing modified data to disk. Write dirty blocks to disk whenever they are
evicted. Because write-behind makes your file system more fragile in the face of crashes,
in addition you should periodically write all dirty, cached blocks back to disk. The cache
should also be written back to disk in `filesys_done()`, so that halting Pintos flushes the
cache.

If you have `timer_sleep()` from the first project working, write-behind is an excellent
application. Otherwise, you may implement a less general facility, but make sure that it
does not exhibit busy-waiting.

You should also implement read-ahead, that is, automatically fetch the next block of
a file into the cache when one block of a file is read, in case that block is about to be
read. Read-ahead is only really useful when done asynchronously. That means, if a process
requests disk block 1 from the file, it should block until disk block 1 is read in, but once
that read is complete, control should return to the process immediately. The read-ahead
request for disk block 2 should be handled asynchronously, in the background.
To do so, you might need to create a dedicated thread for the IO. When the disk
IO needs, the thread passes the request to the dedicated one.

#### Hint for implementation

You might implement this functionality through the virtual memory subsystem as
Linux does. You can introduce new vm type (e.g. "pagecache") and manage
the buffer cache through the `swap_in`, `swap_out`, and `destroy` interfaces.
If you choose this design, you can manage the cache with a PAGE_SIZE granularity
rather than SECTOR_SIZE granularity.
For the synchronization issue for file backed page, you can use the mmaped area
as a cache for them.

**We recommend integrating the cache into your design early.**  In the past, many
groups have tried to tack the cache onto a design late in the design process.
This is very difficult. These groups have often turned in projects that failed
most or all of the tests.
