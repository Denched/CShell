#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "simpleshell.h"

#define MAX_BUFFER 1024 // 1KB enough for user input and small enough for the computer
#define MAX_ARGS 64
#define SEPARATORS " \t\n"

typedef struct
{
    char cmd_name[50];
    // function pointer
    int (*call_command)(char*args[]);

} Command;

// Centralized command table for modularity
Command command_table[] = {
    {"clr",     clear_cmd},
    {"quit",    quit_cmd},
    {"environ", environ_cmd},
    {"pause",   pause_cmd},
    {"cd",      cd_cmd},
    {"echo",    echo_cmd},
    {"help",    help_cmd},
    {"dir",     dir_cmd},
};

const int num_cmds = sizeof(command_table) / sizeof(command_table[0]);


int main(int argc, char**argv)
{

    char buf[MAX_BUFFER];
    char *args[MAX_ARGS];

    // argv[0] defines the executable file
    set_shell_path(argv[0]);
    if (argc > 1)
    {
        return batch_file(argv[1]);
    }
    char *path = getenv("shell");
    char prompt[PATH_MAX];
    if (path == NULL)
    {
        path = "shell";
    }

    int exit = 0;

    // Run till EOF signal
    while (!feof(stdin))
    {

        if (getcwd(prompt, sizeof(prompt)) == NULL)
        {
            perror("getcwd failed");
            strcpy(prompt, "shell"); // fallback
        }

        fputs(prompt, stdout);
        fputs("$ ", stdout);
        // Waits for user to type a cmd
        if (fgets(buf, MAX_BUFFER, stdin))
        {

            buf[strcspn(buf, "\n")] = 0;
            parse_args(args, buf);
            Redirection redir;

            // Parse arguments
            if (parse_redirection(args, &redir) == -1)
            {
                continue;
            }
            int background = parse_background(args);
            if (!args[0])
            {
                continue;
            }
            int builtin_result = 0;

            builtin_result = run_builtin_redir(args, &redir);

            if (builtin_result == 2)
            {
                exit = 1;
            }
            else if (builtin_result == 0)
            {
                exec_fork(args, &redir, background);
            }

        }
        if (exit)
        {
            break;
        }
    }
    return 0;

}

void set_shell_path(const char *exec_name)
{
    char path[PATH_MAX];

    // Gets the absolute path of the executable file

    if (realpath(exec_name, path) == NULL)
    {
        perror("Real path is NULL [realpath]");
        exit(1);
    }

    // Sets shell path and overwrites if there is any already set (1)

    if (setenv("shell", path, 1) != 0)
    {
        perror("Failed to create or update the enviroment variable [setenv]");
        exit(1);
    }

}

void parse_args(char *args[], char buf[])
{
    int arg_index = 0;
    char *token = strtok(buf, SEPARATORS);

    while (token && arg_index < MAX_ARGS - 1)
    {
        args[arg_index++] = token;
        token = strtok(NULL, SEPARATORS);
    }

    args[arg_index] = NULL;
}
int run_command(char *args[])
{
    if (!args[0]) {
        return 0;
    }

    for (int i = 0; i < num_cmds; i++)
    {
        if (command_table[i].call_command == NULL)
            continue;

        if (strcmp(args[0], command_table[i].cmd_name) == 0)
        {
            int value = command_table[i].call_command(args);

            if (value == 1)
                return 2;   // built-in quit cmd

            return 1;       // built-in handled normally
        }
    }

    return 0;   // not a built-in
}


int batch_file(const char *file_name)
{
    Redirection redir;
    FILE *fp = fopen(file_name, "r");
    if (!fp)
    {
        perror("Fopen failed");
        return 1;
    }

    char buf[MAX_BUFFER];
    char *args[MAX_ARGS];

    // Loop till theres no lines left
    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        buf[strcspn(buf, "\r\n")] = 0; // remove any trailing/new line chars, 0 for null terminator
        parse_args(args, buf);
        if (parse_redirection(args, &redir) == -1)
        {
            continue;
        }
        int background = parse_background(args);

        if (!args[0] || args[0][0] == '#') {
            continue;
        }

        int result = run_command(args);

        if (result == 2)
        {
            fclose(fp);
            return 1;
        }
        else if (result == 0)
        {
            exec_fork(args, &redir,background);
        }
    }

    if (ferror(fp))
    {
        perror("Read error [file]");
        fclose(fp);
        return 1;
    }

    fclose(fp);
    return 0;
}
