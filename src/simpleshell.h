#pragma once // Forces header file to be only included once during compilation per .c file

// Struct to make handling redirections easier
typedef struct
{
    char *input_file;
    char *output_file;
    int append;
} Redirection;

int run_command(char *args[]);
// Set absolute path to shell environment variable
void set_shell_path(const char *exec_name);
// Parse arguments from stdin
void parse_args(char *args[], char buf[]);
// Get file, read and execute commands from the file
int batch_file(const char *file_name);

// Cmds
int clear_cmd(char *args[]);
int quit_cmd(char *args[]);
int environ_cmd(char *args[]);
int pause_cmd(char *args[]);
int cd_cmd(char *args[]);
int echo_cmd(char *args[]);
int dir_cmd(char *args[]);
int help_cmd(char *args[]);

// Fork
void exec_fork(char *args[], Redirection *redir, int background);
int parse_redirection(char *args[], Redirection *redir);
int apply_redirection(Redirection *redir);
int run_builtin_redir(char *args[], Redirection *redir);
int parse_background(char *args[]);
