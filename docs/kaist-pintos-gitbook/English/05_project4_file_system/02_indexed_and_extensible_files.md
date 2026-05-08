# 02_indexed_and_extensible_files

## Indexed and Extensible Files

The basic file system allocates files as a single extent, making it vulnerable to external
fragmentation, that is, it is possible that an *n*-block file cannot be allocated even though
*n* blocks are free (i.e. external fragmentation). Eliminate this problem by modifying the
on-disk inode structure.

In practice, this probably means using an index structure with direct, indirect, and doubly
indirect blocks. In previous semesters, most students adopted something like what you have
learned as *Berkeley UNIX FFS* with multi-level indexing. However, to make your life easier,
we make you implement it in an easier way: FAT. **You must implement FAT with given skeleton code.** 
Your code MUST NOT contain any code for multi-level indexing (FFS in the lecture).
You will receive 0pts for file growth parts.

NOTE: You can assume that the file system partition will not be larger than 8 MB. You must
support files as large as the partition (minus metadata). Each inode is stored in one disk
sector, limiting the number of block pointers that it can contain.

### Indexing large files with FAT (File Allocation Table)

> Warning: this document assumes you have already understood the basic principle of
> general filesystem and FAT from the lecture. If not, please go back to the lecture note
> and understand what is filesystem and FAT.

In the basic filesystem that you have used for the previous projects, a file was stored as a
contiguous single chunk over multiple disk sectors. Let's call *contiguous chunk* as *cluster*,
since a cluster (chunk) can contain one or more contiguous disk sectors. In this point of view,
the size of a cluster in the basic file system was equal to the size of a file that stored in the
cluster.

To mitigate external fragmentation, we can shrink the size of cluster (recall the page size in
virtual memory). For simplicity, in our skeleton code, we fixed number of sectors in a cluster
as `1`. When we use smaller clusters like it, a cluster might not enough to store the entire file.
In this case, we need multiple clusters for a file, so we need a data structure to index the clusters
for a file in the inode. One of the easiest way is to use linked-list (a.k.a *chain*). An inode
can contain the sector number of the first block of the file, and the first block
may contain the sector number of the second block. This naïve approach, however, is too slow
because we have to read every block of the file, even though what we really need was only the
last block. To overcome this, FAT (File Allocation Table) puts the connectivity of blocks in
a fixed-size File Allocation Table rather than the blocks themselves. Since FAT only contains the
connectivity value rather than the actual data, its size is small enough to be cached in DRAM.
As a result, we can just read corresponding entries in the table.

You will implement inode indexing with the skeleton code provided in `filesys/fat.c`. This
section briefly describes what already-implemented functions in `fat.c` do and what you
should implement.

First of all, 6 functions in `fat.c` (i.e. `fat_init()`, `fat_open()`, `fat_close()`, `fat_create()`,
and `fat_boot_create()`) initialize and format disks at the boot time, so you don't
have to modify them. However, you'll need to write `fat_fs_init()`, and briefly understanding what they
do could be helpful.

---

```c
cluster_t fat_fs_init (void);
```

> Initialize FAT file system. You have to initialize `fat_length` and `data_start` field of `fat_fs`.
> `fat_length` stores how many clusters in the filesystem and `data_start` stores in which sector
> we can start to store files. You may want to exploit some values stored in `fat_fs->bs`.
> Also, you may want to initialize some other useful data in this function.

---

```c
cluster_t fat_create_chain (cluster_t clst);
```

> Extend a chain by appending a cluster after the cluster specified in `clst` (cluster indexing number).
> If `clst` is equal to zero, then create a new chain. Return the cluster number of newly allocated cluster.

---

```c
void fat_remove_chain (cluster_t clst, cluster_t pclst);
```

> Starting from `clst`, remove clusters from a chain. `pclst` should be the direct previous cluster in the chain.
> This means, after the execution of this function, `pclst` should be the last element of the updated chain.
> If `clst` is the first element in the chain, `pclst` should be 0.

---

```c
void fat_put (cluster_t clst, cluster_t val);
```

> Update FAT entry pointed by cluster number `clst` to `val`. Since each entry in FAT points the next
> cluster in a chain (if exist; otherwise `EOChain`), this could be used to update connectivity.

---

```c
cluster_t fat_get (cluster_t clst);
```

> Return in which cluster number the given cluster `clst` points.

---

```c
disk_sector_t cluster_to_sector (cluster_t clst);
```

> Translates a cluster number `clst` into the corresponding sector number and return the sector number.

---

You may wish to exploit these functions in `filesys.c` and `inode.c` to augment the basic file system.

### File Growth

**Implement extensible files.**  In the basic file system, the file
size is specified when the file is created. In most modern file systems, however, a file is
initially created with size 0 and is then expanded every time a write is made off the end of
the file. Your file system must allow this.

There should be no predetermined limit on the size of a file, except that a file cannot
exceed the size of the file system (minus metadata). This also applies to the root directory
file, which should now be allowed to expand beyond its initial limit of 16 files.

User programs are allowed to seek beyond the current end-of-file (EOF). The seek itself
does not extend the file. Writing at a position past EOF extends the file to the position
being written, and any gap between the previous EOF and the start of the write must be
filled with zeros. A read starting from a position past EOF returns no bytes.

Writing far beyond EOF can cause many blocks to be entirely zero. Some file systems
allocate and write real data blocks for these implicitly zeroed blocks. Other file systems do
not allocate these blocks at all until they are explicitly written. The latter file systems are
said to support "sparse files." You may adopt either allocation strategy in your file system.
