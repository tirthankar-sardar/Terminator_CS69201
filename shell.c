// command execution

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <errno.h>
#include <glob.h>
#include "io_redirect.h"
#include "terminal.h"
#include "shell.h"
#include "gui.h"
#include "variables.h"

int parse_arguments(char *command, char *argv[], int max_args)
{
    static char storage[MAX_PIPE_COMMANDS][INPUT_SIZE];
    static int storage_slot = 0;

    char *token_storage = storage[storage_slot];
    storage_slot = (storage_slot + 1) % MAX_PIPE_COMMANDS;

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
            argv[argc++] = &token_storage[j];
            token_storage[j++] = command[i++];
            token_storage[j++] = '\0';
            continue;
        }

        argv[argc++] = &token_storage[j];

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

            token_storage[j++] = c;
        }

        token_storage[j++] = '\0';
    }

    argv[argc] = NULL;
    return argc;
}

int parse_multiwatch(char *command, char commands[][INPUT_SIZE],
                     int max_commands)
{
    char *p = command;

    while (*p == ' ' || *p == '\t')
        p++;

    if (strncmp(p, "multiWatch", 10) != 0)
        return -1;

    p += 10;

    if (*p != ' ' && *p != '\t' && *p != '[')
        return -1;

    while (*p == ' ' || *p == '\t')
        p++;

    if (*p++ != '[')
        return -1;

    int count = 0;

    while (*p != '\0')
    {
        while (*p == ' ' || *p == '\t' || *p == ',')
            p++;

        if (*p == ']')
        {
            p++;
            break;
        }

        if (*p != '"' && *p != '\'')
            return -1;

        char quote = *p++;
        int length = 0;

        while (*p != '\0' && *p != quote)
        {
            if (*p == '\\' && p[1] != '\0')
                p++;

            if (length >= INPUT_SIZE - 1)
                return -1;

            commands[count][length++] = *p++;
        }

        if (*p != quote)
            return -1;

        p++;

        if (count >= max_commands)
            return -1;

        commands[count][length] = '\0';
        count++;

        while (*p == ' ' || *p == '\t')
            p++;

        if (*p == ']')
        {
            p++;
            break;
        }

        if (*p != ',')
            return -1;
    }

    while (*p == ' ' || *p == '\t')
        p++;

    if (*(p - 1) != ']' || *p != '\0' || count == 0)
        return -1;

    return count;
}

static int start_watch_command(WatchCommand *watch,
                               const char *cwd)
{
    int fd[2];

    if (pipe(fd) == -1)
    {
        perror("multiWatch: pipe");
        return -1;
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        perror("multiWatch: fork");
        close(fd[0]);
        close(fd[1]);
        return -1;
    }

    if (pid == 0)
    {
        close(fd[0]);

        if (dup2(fd[1], STDOUT_FILENO) == -1 ||
            dup2(fd[1], STDERR_FILENO) == -1)
            _exit(1);

        close(fd[1]);

        if (chdir(cwd) == -1)
        {
            perror("multiWatch: chdir");
            _exit(1);
        }

        execl("/bin/sh", "sh", "-c", watch->command,
              (char *)NULL);

        perror("multiWatch: exec");
        _exit(127);
    }

    close(fd[1]);

    watch->child_pids[0] = pid;
    watch->child_count = 1;
    watch->output_fd = fd[0];
    watch->running = 1;
    watch->next_run = time(NULL) + WATCH_INTERVAL;

    return 0;
}

static void start_multiwatch(char commands[][INPUT_SIZE],
                             int count,
                             TerminalTab *tab)
{
    if (tab->multiwatch_active)
    {
        append_output("multiWatch is already active in this tab.\n",
                      41);
        gui_draw();
        return;
    }

    tab->watch_command_count = 0;
    tab->multiwatch_active = 1;

    for (int i = 0; i < count; i++)
    {
        WatchCommand *watch = &tab->watch_commands[i];

        memset(watch, 0, sizeof(*watch));

        snprintf(watch->command, sizeof(watch->command),
                 "%s", commands[i]);

        watch->output_fd = -1;

        if (start_watch_command(watch, tab->cwd) == -1)
        {
            tab->multiwatch_active = 0;

            for (int j = 0; j < i; j++)
            {
                WatchCommand *started = &tab->watch_commands[j];

                if (started->running)
                {
                    kill(started->child_pids[0], SIGTERM);
                    close(started->output_fd);
                    waitpid(started->child_pids[0], NULL, 0);

                    started->output_fd = -1;
                    started->running = 0;
                }
            }

            append_output("multiWatch: failed to start command.\n",
                          38);
            gui_draw();
            return;
        }

        tab->watch_command_count++;
    }

    append_output("multiWatch started.\n", 20);
    gui_draw();
}

static int split_pipeline(char *command, char *parts[], int max_parts)
{
    int count = 0;
    int start = 0;
    char quote = '\0';
    int paren_depth = 0;

    for (int i = 0;; i++)
    {
        char c = command[i];

        if (c == '\0' ||
            (c == '|' && quote == '\0' && paren_depth == 0))
        {
            if (count >= max_parts)
                return -1;

            command[i] = '\0';

            char *part = command + start;

            while (*part == ' ' || *part == '\t')
                part++;

            size_t len = strlen(part);

            while (len > 0 &&
                   (part[len - 1] == ' ' || part[len - 1] == '\t'))
                part[--len] = '\0';

            if (len == 0)
                return -1;

            parts[count++] = part;

            if (c == '\0')
                break;

            start = i + 1;
            continue;
        }

        if (c == '\\' && quote != '\'')
        {
            if (command[i + 1] != '\0')
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

            continue;
        }

        if (quote == '\0')
        {
            if (c == '(' && i > 0 && command[i - 1] == '$')
                paren_depth++;
            else if (c == ')' && paren_depth > 0)
                paren_depth--;
        }
    }

    return count;
}

static int expand_globs(char *argv[], char *expanded[],
                        int max_args, const char *cwd)
{
    int count = 0;

    for (int i = 0; argv[i] != NULL; i++)
    {
        if (strpbrk(argv[i], "*?[") == NULL)
        {
            if (count >= max_args - 1)
                return -1;

            expanded[count++] = argv[i];
            continue;
        }

        char pattern[PATH_MAX * 2];

        if (argv[i][0] == '/')
            snprintf(pattern, sizeof(pattern), "%s", argv[i]);
        else
            snprintf(pattern, sizeof(pattern), "%s/%s", cwd, argv[i]);

        glob_t matches;
        memset(&matches, 0, sizeof(matches));

        int result = glob(pattern, 0, NULL, &matches);

        if (result == 0)
        {
            for (size_t k = 0; k < matches.gl_pathc; k++)
            {
                if (count >= max_args - 1)
                {
                    globfree(&matches);
                    return -1;
                }

                expanded[count++] = strdup(matches.gl_pathv[k]);
            }

            globfree(&matches);
        }
        else
        {
            globfree(&matches);

            if (count >= max_args - 1)
                return -1;

            expanded[count++] = argv[i];
        }
    }

    expanded[count] = NULL;
    return count;
}

static void execute_pipeline(char *command, TerminalTab *tab)
{
    char *parts[MAX_PIPE_COMMANDS];

    int command_count =
        split_pipeline(command, parts, MAX_PIPE_COMMANDS);

    if (command_count <= 0)
    {
        append_output("terminator: invalid pipeline\n", 29);
        tab->last_status = 2;
        gui_draw();
        return;
    }

    char *stage_argv[MAX_PIPE_COMMANDS][64];
    char *expanded_argv[MAX_PIPE_COMMANDS][64];
    IORedirection redirects[MAX_PIPE_COMMANDS];

    /* Parse each command and its redirections. */
    for (int i = 0; i < command_count; i++)
    {
        int argc = parse_arguments(parts[i], stage_argv[i], 64);

        if (argc == 0 ||
            parse_redirections(stage_argv[i], &redirects[i]) != 0 ||
            stage_argv[i][0] == NULL)
        {
            append_output(
                "terminator: invalid command or redirection in pipeline\n",
                54);
            tab->last_status = 2;
            gui_draw();
            return;
        }

        if (expand_globs(stage_argv[i], expanded_argv[i],
                         64, tab->cwd) < 0)
        {
            append_output("terminator: too many arguments\n", 31);
            tab->last_status = 2;
            gui_draw();
            return;
        }
    }

    /* Create N-1 pipes between commands. */
    int pipes[MAX_PIPE_COMMANDS - 1][2];
    int capture[2];
    int made_pipes = 0;

    for (int i = 0; i < command_count - 1; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            for (int j = 0; j < made_pipes; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            append_output("terminator: pipe failed\n", 23);
            tab->last_status = 1;
            gui_draw();
            return;
        }

        made_pipes++;
    }

    /* Extra pipe to capture output for the GUI. */
    if (pipe(capture) == -1)
    {
        for (int i = 0; i < made_pipes; i++)
        {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }

        append_output("terminator: output pipe failed\n", 30);
        tab->last_status = 1;
        gui_draw();
        return;
    }

    pid_t children[MAX_PIPE_COMMANDS];
    int started = 0;

    /* Start one child for every command. */
    for (int i = 0; i < command_count; i++)
    {
        pid_t pid = fork();

        if (pid == -1)
        {
            append_output("terminator: fork failed\n", 23);
            break;
        }

        if (pid == 0)
        {
            /* Read from the previous command. */
            if (i > 0 &&
                dup2(pipes[i - 1][0], STDIN_FILENO) == -1)
                _exit(126);

            /* Write to the next command, or GUI capture pipe. */
            if (i < command_count - 1)
            {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1)
                    _exit(126);
            }
            else
            {
                if (dup2(capture[1], STDOUT_FILENO) == -1)
                    _exit(126);
            }

            /* Capture errors for the GUI too. */
            if (dup2(capture[1], STDERR_FILENO) == -1)
                _exit(126);

            /* Close unused pipe descriptors. */
            for (int j = 0; j < command_count - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            close(capture[0]);
            close(capture[1]);

            if (chdir(tab->cwd) != 0)
            {
                dprintf(STDERR_FILENO,
                        "terminator: cannot change directory\n");
                _exit(126);
            }

            /* Apply explicit input redirection. */
            if (redirects[i].input_file != NULL)
            {
                int in_fd = open(redirects[i].input_file, O_RDONLY);

                if (in_fd == -1)
                {
                    dprintf(STDERR_FILENO,
                            "terminator: cannot open input file: %s\n",
                            redirects[i].input_file);
                    _exit(1);
                }

                if (dup2(in_fd, STDIN_FILENO) == -1)
                    _exit(126);

                close(in_fd);
            }

            /* Apply explicit output redirection. */
            if (redirects[i].output_file != NULL)
            {
                int out_fd = open(
                    redirects[i].output_file,
                    O_WRONLY | O_CREAT | O_TRUNC,
                    0644);

                if (out_fd == -1)
                {
                    dprintf(STDERR_FILENO,
                            "terminator: cannot open output file: %s\n",
                            redirects[i].output_file);
                    _exit(1);
                }

                if (dup2(out_fd, STDOUT_FILENO) == -1)
                    _exit(126);

                close(out_fd);
            }

            execvp(expanded_argv[i][0], expanded_argv[i]);

            int saved_errno = errno;

            dprintf(STDERR_FILENO, "terminator: %s: %s\n",
                    expanded_argv[i][0], strerror(saved_errno));

            _exit(saved_errno == ENOENT ? 127 : 126);
        }

        children[started++] = pid;
    }

    /* Parent must close its copies of all pipe ends. */
    for (int i = 0; i < command_count - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    close(capture[1]);

    if (started == 0)
    {
        close(capture[0]);
        tab->last_status = 1;
        gui_draw();
        return;
    }

    /* Save every child PID for the event loop to reap. */
    tab->child_count = started;

    for (int i = 0; i < started; i++)
        tab->child_pids[i] = children[i];

    tab->pid = children[started - 1];
    tab->output_fd = capture[0];
    tab->process_running = 1;
    tab->last_status = 0;

    gui_draw();
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

static int start_watch_command(WatchCommand *watch,
                               const char *cwd);

static void start_multiwatch(char commands[][INPUT_SIZE],
                             int count,
                             TerminalTab *tab);

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

    if (strcmp(argv[0], "multiWatch") == 0)
    {
        char commands[MAX_WATCH_COMMANDS][INPUT_SIZE];

        int count = parse_multiwatch(
            command, commands, MAX_WATCH_COMMANDS);

        if (count < 0)
        {
            append_output(
                "Usage: multiWatch [\"cmd1\", \"cmd2\", ...]\n",
                (int)strlen(
                    "Usage: multiWatch [\"cmd1\", \"cmd2\", ...]\n"));

            gui_draw();
            return;
        }

        start_multiwatch(commands, count, tab);
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

    // int fd[2];

    // IORedirection redirect;

    // if (parse_redirections(argv, &redirect) != 0 || argv[0] == NULL)
    // {
    //     append_output("terminator: invalid redirection\n", 32);
    //     tab->last_status = 2;
    //     gui_draw();
    //     return;
    // }

    // if (pipe(fd) == -1)
    // {
    //     append_output("terminator: pipe failed\n", 23);
    //     tab->last_status = 1;
    //     gui_draw();
    //     return;
    // }

    // pid_t pid = fork();

    // if (pid == -1)
    // {
    //     close(fd[0]);
    //     close(fd[1]);

    //     append_output("terminator: fork failed\n", 23);
    //     tab->last_status = 1;
    //     gui_draw();
    //     return;
    // }

    // if (pid == 0)
    // {
    //     close(fd[0]);

    //     dup2(fd[1], STDOUT_FILENO);
    //     dup2(fd[1], STDERR_FILENO);
    //     close(fd[1]);

    //     // execvp(argv[0], argv);

    //     if (chdir(tab->cwd) != 0)
    //     {
    //         dprintf(STDERR_FILENO,
    //                 "terminator: cannot change directory\n");
    //         _exit(126);
    //     }

    //     int input_fd = -1;
    //     int output_fd = -1;

    //     if (redirect.input_file != NULL)
    //     {
    //         input_fd = open(redirect.input_file, O_RDONLY);

    //         if (input_fd == -1)
    //         {
    //             dprintf(STDERR_FILENO,
    //                     "terminator: cannot open input file: %s\n",
    //                     redirect.input_file);
    //             _exit(1);
    //         }

    //         if (dup2(input_fd, STDIN_FILENO) == -1)
    //             _exit(1);

    //         close(input_fd);
    //     }

    //     if (redirect.output_file != NULL)
    //     {
    //         output_fd = open(redirect.output_file,
    //                          O_WRONLY | O_CREAT | O_TRUNC,
    //                          0644);

    //         if (output_fd == -1)
    //         {
    //             dprintf(STDERR_FILENO,
    //                     "terminator: cannot open output file: %s\n",
    //                     redirect.output_file);
    //             _exit(1);
    //         }

    //         if (dup2(output_fd, STDOUT_FILENO) == -1)
    //             _exit(1);

    //         close(output_fd);
    //     }

    //     execvp(argv[0], argv);

    //     int saved_errno = errno;

    //     dprintf(STDERR_FILENO,
    //             "terminator: %s: %s\n",
    //             argv[0], strerror(saved_errno));

    //     _exit(saved_errno == ENOENT ? 127 : 126);
    // }

    // close(fd[1]);

    // tab->pid = pid;
    // tab->output_fd = fd[0];
    // tab->process_running = 1;
    // tab->last_status = 0;

    execute_pipeline(command, tab);

    // gui_draw();
}
