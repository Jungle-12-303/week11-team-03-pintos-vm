# 06_deny_write

### Deny Write on Executables

**Deny write on executables.**

Add code to deny writes to files in use as executables. Many OSes do this
because of the unpredictable results if a process tried to run code that was in
the midst of being changed on disk. This is especially important once virtual
memory is implemented in project 3, but it can't hurt even now.

You can use `file_deny_write()` to prevent writes to an open file. Calling
`file_allow_write()` on the file will re-enable them (unless the file is denied
writes by another opener). Closing a file will also re-enable writes. Thus, to
deny writes to a process's executable, you must keep it open as long as the
process is still running.
