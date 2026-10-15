// command execution

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "terminal.h"
#include "shell.h"
#include "gui.h"
#include "variables.h"

int parse_arguments(char *command, char *argv[], int max_args)
{
    int argc = 0;
    int i = 0;
    int j = 0;

    while (command[i] != '\0')
    {
        while (command[i] == ' ' || command[i] == '\t')
            i++;

        if (command[i] == '\0')
            break;

        if (argc >= max_args - 1)
            break;

        argv[argc++] = &command[j];

        char quote = '\0';

        while (command[i] != '\0')
        {
            char c = command[i++];

            if (quote == '\0' && (c == ' ' || c == '\t'))
                break;

            if (c == '\'' || c == '"')
            {
                if (quote == '\0')
                {
                    quote = c;
                    continue;
                }
                else if (quote == c)
                {
                    quote = '\0';
                    continue;
                }
            }

            command[j++] = c;
        }

        command[j++] = '\0';
    }

    argv[argc] = NULL;
    return argc;
}

/*void execute_command(void)
{
    input[input_length] = '\0';

    if (input_length == 0)
        return;

    char command[INPUT_SIZE];
    strcpy(command, input);

    append_output("$ ", 2);
    append_output(command, (int)strlen(command));
    append_output("\n", 1);

    input_length = 0;
    input[0] = '\0';

    // vriables expansion

    char expanded[INPUT_SIZE];

    expand_variables(command, expanded, sizeof(expanded), active_tab);
    strcpy(command, expanded);

    if (handle_assignment(command, active_tab))
        return;

    char *argv[64];
    int argc = parse_arguments(command, argv, 64);

    if (argc == 0)
    {
        gui_draw();
        return;
    }

    if (strcmp(argv[0], "exit") == 0)
    {
        running = 0;
        return;
    }

    if (strcmp(argv[0], "cd") == 0)
    {
        const char *path;

        if (argc == 1)
            path = getenv("HOME");
        else
            path = argv[1];

        if (path == NULL || chdir(path) != 0)
        {
            append_output("cd: unable to change directory\n", 32);
        }

        gui_draw();
        return;
    }

    int fd[2];

    if (pipe(fd) == -1)
    {
        perror("pipe");
        gui_draw();
        return;
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        gui_draw();
        return;
    }

    if (pid == 0)
    {
        close(fd[0]);

        dup2(fd[1], STDOUT_FILENO);
        dup2(fd[1], STDERR_FILENO);

        close(fd[1]);

        execvp(argv[0], argv);

        perror(argv[0]);
        _exit(127);
    }
    else
    {
        close(fd[1]);

        char buffer[1024];
        ssize_t bytes_read;

        while ((bytes_read = read(fd[0], buffer, sizeof(buffer))) > 0)
        {
            append_output(buffer, (int)bytes_read);
        }

        close(fd[0]);

        int status;
        waitpid(pid, &status, 0);

        gui_draw();
    }
}*/

void execute_command(void)
{
    input[input_length] = '\0';

    if (input_length == 0)
        return;

    TerminalTab *tab = &tabs[active_tab];

    if (tab->process_running)
    {
        append_output("A command is already running in this tab.\n", 42);
        gui_draw();
        return;
    }

    char command[INPUT_SIZE];
    strcpy(command, input);

    append_output("$ ", 2);
    append_output(command, (int)strlen(command));
    append_output("\n", 1);

    input_length = 0;
    input[0] = '\0';

    char expanded[INPUT_SIZE];
    expand_variables(command, expanded, sizeof(expanded), active_tab);
    strcpy(command, expanded);

    if (handle_assignment(command, active_tab))
    {
        tab->last_status = 0;
        gui_draw();
        return;
    }

    char *argv[64];
    int argc = parse_arguments(command, argv, 64);

    if (argc == 0)
    {
        gui_draw();
        return;
    }

    if (strcmp(argv[0], "exit") == 0)
    {
        gui_close_tab();
        return;
    }

    /*if (strcmp(argv[0], "cd") == 0)
    {
        const char *path = argc == 1 ? getenv("HOME") : argv[1];

        if (path == NULL || chdir(path) != 0)
        {
            append_output("cd: unable to change directory\n", 31);
            tab->last_status = 1;
        }
        else
        {
            tab->last_status = 0;
        }

        gui_draw();
        return;
    }*/
    if (strcmp(argv[0], "cd") == 0)
    {
        const char *path = argc == 1 ? getenv("HOME") : argv[1];
        char new_path[PATH_MAX];
        char resolved_path[PATH_MAX];
        struct stat st;

        if (path == NULL)
        {
            append_output("cd: HOME not set\n", 17);
            tab->last_status = 1;
        }
        else
        {
            if (path[0] == '/')
                snprintf(new_path, sizeof(new_path), "%s", path);
            else
                snprintf(new_path, sizeof(new_path), "%s/%s",
                         tab->cwd, path);

            if (realpath(new_path, resolved_path) == NULL ||
                stat(resolved_path, &st) != 0 ||
                !S_ISDIR(st.st_mode))
            {
                append_output("cd: unable to change directory\n", 31);
                tab->last_status = 1;
            }
            else
            {
                snprintf(tab->cwd, sizeof(tab->cwd), "%s", resolved_path);
                tab->last_status = 0;
            }
        }

        gui_draw();
        return;
    }

    int fd[2];

    if (pipe(fd) == -1)
    {
        append_output("terminator: pipe failed\n", 23);
        tab->last_status = 1;
        gui_draw();
        return;
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        close(fd[0]);
        close(fd[1]);

        append_output("terminator: fork failed\n", 23);
        tab->last_status = 1;
        gui_draw();
        return;
    }

    if (pid == 0)
    {
        close(fd[0]);

        dup2(fd[1], STDOUT_FILENO);
        dup2(fd[1], STDERR_FILENO);
        close(fd[1]);

        //execvp(argv[0], argv);

        if (chdir(tab->cwd) != 0)
        {
            dprintf(STDERR_FILENO,
                    "terminator: cannot change directory\n");
            _exit(126);
        }

        execvp(argv[0], argv);

        dprintf(STDERR_FILENO,
                "terminator: %s: command not found\n", argv[0]);
        _exit(127);
    }

    close(fd[1]);

    tab->pid = pid;
    tab->output_fd = fd[0];
    tab->process_running = 1;
    tab->last_status = 0;

    gui_draw();
}
