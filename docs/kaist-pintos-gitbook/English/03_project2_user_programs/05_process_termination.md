# 05_process_termination

## Process Termination Message

**Print out the process termination message**

Whenever a user process terminates, because it called `exit` or for any other
reason, **print the process's name and exit code**, formatted as if printed by

```c
printf ("%s: exit(%d)\n", ...);
```

The name printed should be the full name passed to `fork()`. Do not print these
messages when a kernel thread that is not a user process terminates, or when the
halt system call is invoked. The message is optional when a process fails to
load.

Aside from this, don't print any other messages that Pintos as provided doesn't
already print. You may find extra messages useful during debugging, but they
will confuse the grading scripts and thus lower your score.
