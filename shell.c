// command execution

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "io_redirect.h"
#include "terminal.h"
#include "shell.h"
#include "gui.h"
#include "variables.h"


int parse_arguments(char *command, char *argv[], int max_args)
{
    static char storage[INPUT_SIZE];

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

        if (command[i] == '<' || command[i] == '>')
        {
            argv[argc++] = &storage[j];
            storage[j++] = command[i++];
            storage[j++] = '\0';
            continue;
        }

        argv[argc++] = &storage[j];

        char quote = '\0';

        while (command[i] != '\0')
        {
            char c = command[i];

            if (quote == '\0' &&
                (c == ' ' || c == '\t' || c == '<' || c == '>'))
                break;

            i++;

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

            storage[j++] = c;
        }

        storage[j++] = '\0';
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

int command_needs_more_input(const char *command)
{
    char quote = '\0';

    for (int i = 0; command[i] != '\0'; i++)
    {
        char c = command[i];

        if (c == '\\' && quote != '\'')
        {
            if (command[i + 1] == '\0')
                return 1;

            i++;
            continue;
        }

        if (quote == '\'')
        {
            if (c == '\'')
                quote = '\0';
            continue;
        }

        if (c == '\'' && quote == '\0')
        {
            quote = '\'';
            continue;
        }

        if (c == '"')
        {
            if (quote == '"')
                quote = '\0';
            else if (quote == '\0')
                quote = '"';
        }
    }

    return quote != '\0';
}

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

    int read_pos = 0;
    int write_pos = 0;

    while (command[read_pos] != '\0')
    {
        if (command[read_pos] == '\\' &&
            command[read_pos + 1] == '\n')
        {
            read_pos += 2;
        }
        else
        {
            command[write_pos++] = command[read_pos++];
        }
    }

    command[write_pos] = '\0';

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
    dprintf(STDERR_FILENO, "DEBUG argc=%d\n", argc);

    for (int i = 0; i < argc; i++)
    {
        dprintf(STDERR_FILENO, "DEBUG argv[%d]=[%s]\n", i, argv[i]);
    }

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

    IORedirection redirect;

    if (parse_redirections(argv, &redirect) != 0 || argv[0] == NULL)
    {
        append_output("terminator: invalid redirection\n", 32);
        tab->last_status = 2;
        gui_draw();
        return;
    }

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

        // execvp(argv[0], argv);

        if (chdir(tab->cwd) != 0)
        {
            dprintf(STDERR_FILENO,
                    "terminator: cannot change directory\n");
            _exit(126);
        }

        int input_fd = -1;
        int output_fd = -1;

        if (redirect.input_file != NULL)
        {
            input_fd = open(redirect.input_file, O_RDONLY);

            if (input_fd == -1)
            {
                dprintf(STDERR_FILENO,
                        "terminator: cannot open input file: %s\n",
                        redirect.input_file);
                _exit(1);
            }

            if (dup2(input_fd, STDIN_FILENO) == -1)
                _exit(1);

            close(input_fd);
        }

        if (redirect.output_file != NULL)
        {
            output_fd = open(redirect.output_file,
                             O_WRONLY | O_CREAT | O_TRUNC,
                             0644);

            if (output_fd == -1)
            {
                dprintf(STDERR_FILENO,
                        "terminator: cannot open output file: %s\n",
                        redirect.output_file);
                _exit(1);
            }

            if (dup2(output_fd, STDOUT_FILENO) == -1)
                _exit(1);

            close(output_fd);
        }

        execvp(argv[0], argv);

        int saved_errno = errno;

        dprintf(STDERR_FILENO,
                "terminator: %s: %s\n",
                argv[0], strerror(saved_errno));

        _exit(saved_errno == ENOENT ? 127 : 126);
    }

    close(fd[1]);

    tab->pid = pid;
    tab->output_fd = fd[0];
    tab->process_running = 1;
    tab->last_status = 0;

    gui_draw();
}
