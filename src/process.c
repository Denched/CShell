#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include "simpleshell.h"

void exec_fork(char *args[], Redirection *redir, int background)
{
    // Create clone of program
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Fork has not succeeded");
        return;
    }
    else if (pid == 0) // child process runs external cmds on copy of simpleshell
    {

        // Set child processes parent shell path
        char *shell_path = getenv("shell");
        if (shell_path != NULL)
        {
            setenv("parent", shell_path, 1);
        }
        if (apply_redirection(redir) == -1)
        {
            perror("Redirection has failed");
            exit(1);
        }

        if (execvp(args[0], args) == -1) // run external cmds by replacing the child program's image with new program
        {
            perror("Execution has failed");
            exit(1);
        }
    }
    else
    {
        // This will run the child asynchronously and prevent blocking the parent shell.
        if (background)
        {
            // printf("The background pid is %d\n", pid);
            fflush(stdout);
        }
        else
        {
            int status;
            waitpid(pid, &status, 0); // hang until the cmd is finished in the child process
        }
    }
}

int parse_redirection(char *args[], Redirection *redir)
{
    int i = 0; // read pointer
    int j = 0; // write pointer

    redir->input_file = NULL;
    redir->output_file = NULL;
    redir->append = 0;

    while (args[i] != NULL)
    {
        if (!strcmp(args[i], "<"))
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "Missing input file\n");
                return -1;
            }
            redir->input_file = args[i + 1];
            i += 2; // Jump two spaces to make sure we dont reprocess the file we just got from pos i + 1
        }
        else if (!strcmp(args[i], ">"))
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "Missing output file\n");
                return -1;
            }
            redir->output_file = args[i + 1];
            redir->append = 0;
            i += 2;
        }
        else if (!strcmp(args[i], ">>"))
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "Missing output file\n");
                return -1;
            }
            redir->output_file = args[i + 1];
            redir->append = 1;
            i += 2;
        }
        else
        {
            args[j++] = args[i++]; // Only move forward when we find a non redirection word
        }
    }

    args[j] = NULL; // NULL to make sure we stop after last valid command
    return 0;
}
int apply_redirection(Redirection *redir)
{
    if (redir->input_file != NULL)
    {
        // Close + open existing stream "r" = read file + reassign stream to new file
        if (freopen(redir->input_file, "r", stdin) == NULL)
        {
            perror("Input redirection failed");
            return -1;
        }
    }

    if (redir->output_file != NULL)
    {
        const char *mode = redir->append ? "a" : "w";
        // Close + open existing stream "a"/"w" append or overwrite file + reassign stream to new file
        if (freopen(redir->output_file, mode, stdout) == NULL)
        {
            perror("Output redirection failed");
            return -1;
        }
    }

    return 0;
}


// Need this function to be able to redirect stdout/stdin for internal commands in the parent shell
int run_builtin_redir(char *args[], Redirection *redir)
{
    // Backup copy of stdin and stdout file descriptors(ID to keep track of data streams)
    int saved_stdin = -1;
    int saved_stdout = -1;

    // Holds opened input/output descriptors
    int fd_in = -1;
    int fd_out = -1;

    if (redir->input_file != NULL)
    {
        saved_stdin = dup(STDIN_FILENO); // duplicate stdin, preserves original shell stdin to be restored later
        if (saved_stdin == -1)
        {
            perror("dup stdin failed");
            return -1;
        }

        fd_in = open(redir->input_file, O_RDONLY); // Open file, READ ONLY
        if (fd_in == -1)
        {
            perror("Input redirection failed");
            close(saved_stdin); // Making sure to close any descriptors to prevent leaks
            return -1;
        }

        // Replace stdin with fdin, so reads from stdin come from the file now
        if (dup2(fd_in, STDIN_FILENO) == -1)
        {
            perror("dup2 stdin failed");
            close(fd_in);
            close(saved_stdin);
            return -1;
        }

        close(fd_in);
    }

    if (redir->output_file != NULL)
    {
        saved_stdout = dup(STDOUT_FILENO);
        if (saved_stdout == -1)
        {
            perror("dup stdout failed");
            if (saved_stdin != -1) // Close any previously saved stdin before this
            {
                close(saved_stdin);
            };
            return -1;
        }

        int flags = O_WRONLY | O_CREAT; // write, create if file is nil
        if (redir->append)
        {
            flags |= O_APPEND;
        } else
        {
            flags |= O_TRUNC; // Deletes everything in file first
        }

        fd_out = open(redir->output_file, flags, 0644); // open with flags and 0644 permissinos(read write 0644 owner(6) group(4) everyone(4))
        if (fd_out == -1)
        {
            perror("Output redirection failed");
            if (saved_stdin != -1) close(saved_stdin);
            close(saved_stdout);
            return -1;
        }

        // Replace stdout with fdout, command output goes directly to file now
        if (dup2(fd_out, STDOUT_FILENO) == -1)
        {
            perror("dup2 stdout failed");
            close(fd_out);
            if (saved_stdin != -1) close(saved_stdin);
            close(saved_stdout);
            return -1;
        }

        close(fd_out);
    }

    // Now run the commands
    int result = run_command(args);

    // Directly flush  buffered redirected output to ensure it is written
    fflush(stdout);
    fflush(stderr);

    // Restore original descriptors and close them
    if (saved_stdin != -1)
    {
        if (dup2(saved_stdin, STDIN_FILENO) == -1)
        {
            perror("restore stdin failed");
        }

        close(saved_stdin);
    }

    if (saved_stdout != -1)
    {
        if (dup2(saved_stdout, STDOUT_FILENO) == -1)
        {
            perror("restore stdout failed");

        }
        close(saved_stdout);
    }

    return result;
}

int parse_background(char *args[])
{
    int i = 0;

    // Count how many args
    while (args[i] != NULL)
    {
        i++;
    }

    // Check if last one is &.
    if (i > 0 && strcmp(args[i - 1], "&") == 0)
    {
        // Replace it with null
        args[i - 1] = NULL;
        return 1; // Run the program in the background
    }

    return 0;
}
