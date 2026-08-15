#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <libgen.h>

#include "simpleshell.h"

extern char **environ;

int clear_cmd(char *args[])
{
    system("clear");
    return 0; // Continues
}


int quit_cmd(char *args[])
{
    return 1; // Handles break in main file
}

int environ_cmd(char *args[])
{
    for (int i = 0; environ[i] != NULL; i++)
    {
        printf("%s\n", environ[i]);
    }
    return 0;
}

int pause_cmd(char *args[])
{
    printf("Press ENTER to continue...");
    fflush(stdout); // Put print statement instantly on screen

    // Wait till a newline (Enter key), Reads and discards other letters
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    return 0;
}

int cd_cmd(char *args[])
{
    char *path = args[1] ? args[1] : getenv("HOME");


    // Save curr directory
    char oldpwd[PATH_MAX];
    getcwd(oldpwd, sizeof(oldpwd));


    // Attempt to change directory
    if (chdir(path) == -1)
    {
        perror("Path does not exist [cd]");
        return 0;
    }

    // Saves new directory after chdir passed
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));


    // Update new environment variables
    setenv("OLDPWD", oldpwd, 1);
    setenv("PWD", cwd, 1);

    return 0;

}

int echo_cmd(char *args[])
{
    // Args already parsed nicely
    for (int i = 1; args[i] != NULL; i++)
    {
        fputs(args[i], stdout);

        if (args[i+1] != NULL)
            fputc(' ', stdout);
    }

    fputc('\n', stdout);
    fflush(stdout);
    return 0;
}

int dir_cmd(char *args[])
{
    const char *path = args[1] ? args[1] : ".";

    char command[PATH_MAX];

    // This builds the command ls -al <directory> the quotes within help if there are spaces in the path, keeps it as one argument
    if (snprintf(command, sizeof(command), "ls -al \"%s\"", path) >= (int)sizeof(command)) {
        fprintf(stderr, "Path too long [dir]\n");
        return 1;
    }

    int status = system(command);

    if (status == -1) {
        perror("Command failed [dir]");
        return 1;
    }
    return 0;
}

int help_cmd(char *args[])
{
    char *shell_path = getenv("shell");
    if (!shell_path) {
        fprintf(stderr, "Shell variable not set\n");
        return 1;
    }

    // We make a copy into a buffer because dirname modifies its arguments
    char buf[PATH_MAX];
    snprintf(buf, sizeof(buf), "%s", shell_path);

    char *dir = dirname(buf);  // Get directory of where help file lives in

    char helpfile[PATH_MAX];
    snprintf(helpfile, sizeof(helpfile), "%s/../manual/readme.txt", dir);

    FILE *fp = fopen(helpfile, "r");
    if (!fp) {
        perror("[help] cannot open manual");
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        fputs(line, stdout);
    }

    fclose(fp);
    return 0;
}

