<div align="center">
  <h1><b>CShell</b></h1>
  <p>A Unix shell in C, built directly on the POSIX process API with fork/exec, I/O redirection, and background jobs with no library doing the interesting parts</p>
</div>

<!-- <div align="center">
  <img src="./assets/demo.gif" width="800" alt="CShell demo" />
</div> -->

---

## Overview

Standard C cannot spawn or manage processes. system() which is apart of ISO C, hands command strings to the command processor. All key functions (fork, execvp,waitpid, dup2, pipe) that support process management are not ISO C, they're POSIX.

Building a shell teaches the API better than reading about it. Redirecting output to files, running commands in the background, checking how commands exit, are beyond portable C.

The core operations of the shell's spawning of a process are usually: forking, and running the target program to replace the child; the child executes (execvp) the program and the parent waits (waitpid) for the child.

None of this was a novel idea. Mainly built for practice and to deepen my understanding of operating systems.

## Getting started

This project requires a POSIX system. It uses `fork()`, `<sys/wait.h>` and `<unistd.h>`, none of which exist on native Windows - build under Linux, macOS, or WSL.

```bash
git clone https://github.com/Denched/CShell.git
cd CShell
make
./bin/simpleshell
```


```bash
./bin/simpleshell script.txt
```

## What it supports

| Feature | Example |
|---|---|
| External commands | `ls -al` |
| Output redirection | `ls > out.txt` |
| Append | `echo hi >> out.txt` |
| Input redirection | `sort < names.txt` |
| Combined | `sort < names.txt > sorted.txt` |
| Background execution | `sleep 10 &` |
| Batch scripts | `./bin/simpleshell script.txt` |

Built-ins: `cd`, `dir`, `echo`, `clr`, `environ`, `pause`, `help`, `quit`.

Redirection works for built-ins as well as external commands, which takes more work than it sounds like.

## How it works

A line moves through four stages:

```
"ls -l > out.txt"
   ──parse_args────────▶  ["ls", "-l", ">", "out.txt"]
   ──parse_redirection─▶  ["ls", "-l"]  +  { output_file: "out.txt" }
   ──run_command───────▶  not a built-in, so:
   ──exec_fork─────────▶  fork() ─┬─ parent: waitpid()
                                  └─ child:  dup2() ──▶ execvp()
```

**1. Tokenise**: `strtok` parses whitespaces into a NULL terminated `argv` array that `execvp` expects.

**2. Extract Redirects**: The signs `<`, `>` and `>>` are moved to a struct and deleted from the array of arguments since `execvp` should never see them. Done in-place using read and write pointers, so that the kept tokens are placed as compact as possible at the front.

**3. Dispatch**: The name of the command is checked in a table of `{name, function pointer}` pairs. If a match is found, it runs in the shell. Otherwise, it falls through to the fork path.

**4. Fork and exec**: `fork` is called once but returns twice, once in parent, once in child , and the only difference is the return value of the function. The child repositions its file descriptors and calls `execvp` which will substitute the running program and will never be returned successfully. The parent either blocks on `waitpid` or, for a background job, does not.

## Decisions

**Function Pointer Table over if/else chain**  Built-ins are stored in a Command[] array of name-function pairs, so adding one is done in a single line and the dispatch loop never changes.

**Built-ins execute in parent, external commands in child.** This is not a style choice. cd must change the *shell's* working directory. If run in a forked child, the chdir will die with the child, and the parent will stay in the same directory. Therefore built-ins must run in the shell process.

## What I'd change

- **Reap background children.** No backgrounded job ever calls waitpid, so each one is a parent zombie and stays that way until the shell exits. This is fixed by having a `SIGCHLD` handler that corresponds to a waitpid of -1.

- **Separate errors from control flow.** Built-ins return 0 to keep going, and 1 to exit, however some built-ins return 1 to indicate they failed, so an error from a built-in can exit the shell.

- **Process groups.** Without these terminal delivers `SIGINT`. Ctrl-C kills the shell, and not the command being executed. For us to have real job control and term stops, we need to use setpgid and tcsetpgrp.

## Status

A learning project for building process control from scratch. It works and is usable, but it implements a small fraction of the POSIX shell language and is not a replacement for `sh`.
