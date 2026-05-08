# 07_development_tools

# Development Tools

Here are some tools that you might find useful while developing code.

## Tags

Tags are an index to the functions and global variables declared in a program.
Many editors, including Emacs and vi, can use them. The `Makefile` in
`pintos/` produces Emacs-style tags with the command make `TAGS` or vi-style
tags with make tags.

In Emacs, use `M-`. to follow a tag in the current window, `C-x 4` . in a new
window, or `C-x 5` . in a new frame. If your cursor is on a symbol name for any
of those commands, it becomes the default target. If a tag name has multiple
definitions, `M-0 M-`. jumps to the next one. To jump back to where you were
before you followed the last tag, use `M-*`.

## cscope

The `cscope` program also provides an index to functions and variables declared
in a program. It has some features that tag facilities lack. Most notably, it
can find all the points in a program at which a given function is called.

The `Makefile` in `pintos/` produces `cscope` indexes when it is invoked as
make `cscope`. Once the index has been generated, run `cscope` from a shell
command line; no command-line arguments are normally necessary. Then use the
arrow keys to choose one of the search criteria listed near the bottom of the
terminal, type in an identifier, and hit `Enter`. `cscope` will then display
the matches in the upper part of the terminal. You may use the arrow keys to
choose a particular match; if you then hit `Enter`, `cscope` will invoke the
default system editor []()</a>[1](#fn_1)</sup> and position the cursor on that match. To start a
new search, type `Tab`. To exit `cscope`, type `Ctrl-d`.

Emacs and some versions of vi have their own interfaces to `cscope`. For
information on how to use these interface, visit the `cscope` home page
([http://cscope.sourceforge.net](http://cscope.sourceforge.net)).

[]()</a>

> <sup>1</sup>. This is typically `vi`. To exit `vi`, type `:q` `Enter`.[↩](#reffn_1)
