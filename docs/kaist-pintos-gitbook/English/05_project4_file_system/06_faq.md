# 06_faq

# FAQ

### Can BLOCK_SECTOR_SIZE change?

No, BLOCK_SECTOR_SIZE is fixed at 512. For IDE disks, this value is a fixed
property of the hardware. Other disks do not necessarily have a 512-byte sector,
but for simplicity Pintos only supports those that do.

### Indexed Files FAQ

#### What is the largest file size that we are supposed to support?

The file system partition we create will be 8 MB or smaller. However, individual
files will have to be smaller than the partition to accommodate the metadata.
You'll need to consider this when deciding your inode organization.

### Subdirectories FAQ

#### How should a file name like `a//b` be interpreted?

Multiple consecutive slashes are equivalent to a single slash, so this file name is
the same as `a/b`.

#### How about a file name like `/../x`?

The root directory is its own parent, so it is equivalent to `/x`.

#### How should a file name that ends in `/` be treated?

Most Unix systems allow a slash at the end of the name for a directory, and
reject other names that end in slashes. We will allow this behavior, as well as
simply rejecting a name that ends in a slash.

### Buffer Cache FAQ

#### Can we keep a `struct inode_disk` inside `struct inode`?

The goal of the 64-block limit is to bound the amount of cached file system data.
If you keep a block of disk data - whether file data or metadata - anywhere in
kernel memory then you have to count it against the 64-block limit. The same
rule applies to anything that's "similar" to a block of disk data, such as a
`struct inode_disk` without the `length` or `sector_cnt` members.

That means you'll have to change the way the inode implementation accesses its
corresponding on-disk inode right now, since it currently just embeds a `struct inode_disk` in `struct inode` and reads the corresponding sector from disk
when it's created. Keeping extra copies of inodes would subvert the 64-block
limitation that we place on your cache.

You can store a pointer to inode data in `struct inode`, but if you do so you
should carefully make sure that this does not limit your OS to 64 simultaneously
open files. You can also store other information to help you find the inode when
you need it. Similarly, you may store some metadata along each of your 64 cache
entries.

You can keep a cached copy of the free map permanently in memory if you like.
It doesn't have to count against the cache size.

`byte_to_sector()` in `filesys/inode.c` uses the `struct inode_disk`
directly, without first reading that sector from wherever it was in the
storage hierarchy. This will no longer work. You will need to change
`inode_byte_to_sector()` to obtain the `struct inode_disk` from the cache
before using it.
