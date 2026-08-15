Simple Shell Help
-----------------

Built-in commands:

    cd <dir>     - change directory
    dir [path]   - list contents of directory
    echo <text>  - print text
    clr          - clear screen
    environ      - lists environment variables
    pause        - wait for Enter key
    help         - show this help manual
    quit         - exit the shell


Redirection:

> - Redirect standard output to a file (overwrite existing contents)
< - Redirect standard input from a file instead of keyboard input.
>> - Redirect standard output to a file in append mode (keep existing contents).

Examples:
echo hello > out.txt
echo < t.txt
echo hi >> out.txt
sort < names.txt > sorted.txt

Background:
& at the end of a command runs that command in the background.
The shell returns to the prompt immediately instead of waiting for the command to finish in the foreground.

Example:
sleep 10 &
