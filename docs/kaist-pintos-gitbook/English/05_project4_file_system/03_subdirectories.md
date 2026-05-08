# 03_subdirectories

### Subdirectories

**Implement a hierarchical name space.** 
In the basic file system, all files live in a single directory. Modify this to
allow directory entries to point to files or to other directories.

Make sure that directories can expand beyond their original size just as any other file
can.

The basic file system has a 14-character limit on file names. You may retain this limit
for individual file name components, or may extend it, at your option. You must allow full
path names to be much longer than 14 characters.

**Maintain a separate current directory for each process.** 
At startup, set the root as the initial process's current directory. When one
process starts another with the fork system call, the child process inherits its
parent's current directory. After that, the two processes' current directories
are independent, so that either changing its own current directory has no effect
on the other. (This is why, under Unix, the cd command is a shell built-in, not
an external program.)

Update the existing system calls so that, anywhere a file name is provided by
the caller, an absolute or relative path name may used. The directory separator
character is forward slash ('/'). You must also support special file names '.'
and '..', which have the same meanings as they do in Unix.

Update the `open` system call so that it can also open directories. Of the
existing system calls, only close needs to accept a file descriptor for a
directory.

Update the `remove` system call so that it can delete empty directories (other
than the root) in addition to regular files. Directories may only be deleted if
they do not contain any files or subdirectories (other than '.' and '..'). You may decide whether to allow deletion
of a directory that is open by a process or in use as a processs current working directory.
If it is allowed, then attempts to open files (including '.' and '..') or create new files in a
deleted directory must be disallowed.
Implement the following new system calls:

---

```c
bool chdir (const char *dir);
```

> Changes the current working directory of the process to dir, which may be relative
> or absolute. Returns true if successful, false on failure.

---

```c
bool mkdir (const char *dir);
```

> Creates the directory named dir, which may be relative or absolute. Returns true if
> successful, false on failure. Fails if dir already exists or if any directory name in dir,
> besides the last, does not already exist. That is, `mkdir("/a/b/c")` succeeds only if `/a/b` already exists and `/a/b/c` does not.

---

```c
bool readdir (int fd, char *name);
```

> Reads a directory entry from file descriptor fd, which must represent a directory. If
> successful, stores the null-terminated file name in name, which must have room for
> `READDIR_MAX_LEN + 1` bytes, and returns true. If no entries are left in the directory,
> returns false.
>
> . and .. should not be returned by `readdir`.
> If the directory changes while it is open, then it is acceptable for some entries not to
> be read at all or to be read multiple times. Otherwise, each directory entry should
> be read once, in any order.
>
> `READDIR_MAX_LEN` is defined in `lib/user/syscall.h`. If your file system supports
> longer file names than the basic file system, you should increase this value from the
> default of 14.

---

```c
bool isdir (int fd);
```

> Returns true if fd represents a directory, false if it represents an ordinary file.

---

```c
int inumber (int fd);
```

> Returns the inode number of the inode associated with fd, which may represent an ordinary file or a directory.
>
> An inode number persistently identifies a file or directory. It is unique during the
> files existence. In Pintos, the sector number of the inode is suitable for use as an inode number.

### Soft Link

**Implement the soft link machanism in pintos.** 
Soft link (aka. Symbolic link) is a pseudo file object that refers another file
or directory. This file contains information of path of pointed file in terms of
absolute or relative path. Let's assume the following situation:

```
/
├── a
│   ├── link1 -> /file
│   │
│   └── link2 -> ../file
└── file
```

The soft-link named `link1` in `/a` points `/file` (as an absolute path),
and `link2` in `/a` points `../file` (as a relative path).
Reading the `link1` or `link2` file is equivalent to reading `/file`.

---

```c
int symlink (const char *target, const char *linkpath);
```

> Creates a symbolic link named linkpath which contains the string target.
> On success, zero is returned. Otherwise, -1 is returned.
