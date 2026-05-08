# 03_grading

# Grading

We will grade your assignments based on test results.

## Testing

Your test result grade will be based on our tests.
Each project has several tests, each of which has a name beginning with `tests`.
To completely test your submission, invoke make check from the project `build`
directory. This will build and run each test and print a `pass` or `fail`
message for each one. When a test fails, make check also prints some details
of the reason for failure. After running all the tests, make check also prints a
summary of the test results.
You can also run individual tests one at a time. A given test `t` writes its
output to `t.output`, then a script scores the output as `pass` or `fail` and
writes the verdict to `t.result`. To run and grade a single test, make the
`.result` file explicitly from the `build` directory, e.g.
`make tests/threads/alarm-multiple.result`. If make says that the test result is
up-to-date, but you want to re-run it anyway, either run make clean or delete
the `.output` file by hand. By default, each test provides feedback only at
completion, not during its run. If you prefer, you can observe the progress of
each test by specifying `VERBOSE=1` on the make command line, as in make check
`VERBOSE=1`.
All of the tests and related files are in `pintos/src/tests`. Before we test
your submission, we will replace the contents of that directory by a pristine,
unmodified copy, to ensure that the correct tests are used. Thus, you can modify
some of the tests if that helps in debugging, but we will run the originals.
All software has bugs, so some of our tests may be flawed. If you think a test
failure is a bug in the test, not a bug in your code, please point it out.
We will look at it and fix it if necessary. Please don't try to take advantage
of our generosity in giving out our test suite. Your code has to work properly
in the general case, not just for the test cases we supply. For example, it
would be unacceptable to explicitly base the kernel's behavior on the name of
the running test case. Such attempts to side-step the test cases will receive
no credit. If you think your solution may be in a gray area here,
please ask us about it.

## Submission

We provide a command to make an archive file. Go to your pintos project root
directory where you have `threads`, `userprog`, `vm`, `filesys` directory.
Type `TEAM=YOUR_TEAM_NUMBER make archive` (e.g. `TEAM=5 make archive`). Then,
you will have `team5.tar.gz` in the same directory. Submit the archive file.
**DO NOT** make duplicate submissions. One submission for a team is enough.
