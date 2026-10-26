/*#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>


// it takes the keyboard input from the terminal
int main()
{
    Display *display;
    Window window;
    XEvent event;

    display = XOpenDisplay(NULL);

    if (display == NULL)
    {
        printf("Cannot open X display\n");
        return 1;
    }

    int screen = DefaultScreen(display);

    window = XCreateSimpleWindow(
        display,
        RootWindow(display, screen),
        100,
        100,
        600,
        400,
        1,
        BlackPixel(display, screen),
        WhitePixel(display, screen));

    XStoreName(display, window, "Terminator");

    XSelectInput(display, window, ExposureMask | KeyPressMask);

    XMapWindow(display, window);

    while (1)
    {
        XNextEvent(display, &event);

        if (event.type == Expose)
        {
            XDrawString(
                display,
                window,
                DefaultGC(display, screen),
                50,
                100,
                "Type something:",
                15);
        }

        if (event.type == KeyPress)
        {
            char text[10];
            KeySym key;

            int length = XLookupString(
                &event.xkey,
                text,
                sizeof(text),
                &key,
                NULL);

            if (length > 0)
            {
                printf("You typed: %s\n", text);
            }

            if (key == XK_Escape)
            {
                break;
            }
        }
    }

    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}


//it takes the keyboard input from the terminal and displays it in the window
//Backspace deletes the last character,
//Enter clears the input,
//and Escape closes the program.
int main()
{
    Display *display;
    Window window;
    XEvent event;

    char input[256];
    int input_length = 0;

    input[0] = '\0';

    display = XOpenDisplay(NULL);

    if (display == NULL)
    {
        printf("Cannot open X display\n");
        return 1;
    }

    int screen = DefaultScreen(display);

    window = XCreateSimpleWindow(
        display,
        RootWindow(display, screen),
        100,
        100,
        600,
        400,
        1,
        BlackPixel(display, screen),
        WhitePixel(display, screen)
    );

    XStoreName(display, window, "Terminator");

    XSelectInput(
        display,
        window,
        ExposureMask | KeyPressMask
    );

    XMapWindow(display, window);

    while (1)
    {
        XNextEvent(display, &event);

        if (event.type == Expose)
        {
            XClearWindow(display, window);

            XDrawString(
                display,
                window,
                DefaultGC(display, screen),
                20,
                40,
                "$ ",
                2
            );

            XDrawString(
                display,
                window,
                DefaultGC(display, screen),
                35,
                40,
                input,
                input_length
            );
        }

        if (event.type == KeyPress)
        {
            char text[10];
            KeySym key;

            int length = XLookupString(
                &event.xkey,
                text,
                sizeof(text),
                &key,
                NULL
            );

            if (key == XK_Escape)
            {
                break;
            }

            if (key == XK_BackSpace)
            {
                if (input_length > 0)
                {
                    input_length--;
                    input[input_length] = '\0';
                }
            }
            else if (key == XK_Return)
            {
                printf("Command entered: %s\n", input);

                input_length = 0;
                input[0] = '\0';
            }
            else if (length > 0)
            {
                if (input_length + length < 255)
                {
                    for (int i = 0; i < length; i++)
                    {
                        input[input_length] = text[i];
                        input_length++;
                    }

                    input[input_length] = '\0';
                }
            }

            XClearWindow(display, window);

            XDrawString(
                display,
                window,
                DefaultGC(display, screen),
                20,
                40,
                "$ ",
                2
            );

            XDrawString(
                display,
                window,
                DefaultGC(display, screen),
                35,
                40,
                input,
                input_length
            );
        }
    }

    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#define INPUT_SIZE 256
#define OUTPUT_SIZE 32768

Display *display;
Window window;
GC gc;

char input[INPUT_SIZE];
int input_length = 0;

char output[OUTPUT_SIZE];
int output_length = 0;

int running = 1;

void append_output(const char *text, int length)
{
    if (length <= 0)
        return;

    int available = OUTPUT_SIZE - output_length - 1;

    if (available <= 0)
        return;

    if (length > available)
        length = available;

    memcpy(output + output_length, text, length);
    output_length += length;
    output[output_length] = '\0';
}

void draw_window()
{
    XClearWindow(display, window);

    int x = 15;
    int y = 25;
    int line_height = 18;
    int start = 0;

    for (int i = 0; i <= output_length; i++)
    {
        if (output[i] == '\n' || output[i] == '\0')
        {
            int length = i - start;

            if (length > 0)
            {
                XDrawString(
                    display, window, gc,
                    x, y, output + start, length
                );
            }

            y += line_height;
            start = i + 1;
        }
    }

    if (y > 385)
        y = 385;

    XDrawString(display, window, gc, x, y, "$ ", 2);

    XDrawString(
        display, window, gc,
        x + 20, y, input, input_length
    );

    XFlush(display);
}

void execute_command()
{
    input[input_length] = '\0';

    if (input_length == 0)
        return;

    char command[INPUT_SIZE];
    strcpy(command, input);

    append_output("$ ", 2);
    append_output(command, strlen(command));
    append_output("\n", 1);

    input_length = 0;
    input[0] = '\0';

    char *argv[64];
    int argc = 0;

    char *token = strtok(command, " \t");

    while (token != NULL && argc < 63)
    {
        argv[argc] = token;
        argc++;

        token = strtok(NULL, " \t");
    }

    argv[argc] = NULL;

    if (argc == 0)
    {
        draw_window();
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
            perror("cd");
        }

        draw_window();
        return;
    }

    int fd[2];

    if (pipe(fd) == -1)
    {
        perror("pipe");
        draw_window();
        return;
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        draw_window();
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

        draw_window();
    }
}

int main()
{
    display = XOpenDisplay(NULL);

    if (display == NULL)
    {
        fprintf(stderr, "Cannot open X display\n");
        return 1;
    }

    int screen = DefaultScreen(display);

    window = XCreateSimpleWindow(
        display,
        RootWindow(display, screen),
        100, 100,
        700, 450,
        1,
        BlackPixel(display, screen),
        WhitePixel(display, screen)
    );

    XStoreName(display, window, "Terminator");

    XSelectInput(
        display,
        window,
        ExposureMask | KeyPressMask
    );

    gc = DefaultGC(display, screen);

    XMapWindow(display, window);

    input[0] = '\0';
    output[0] = '\0';

    while (running)
    {
        XEvent event;
        XNextEvent(display, &event);

        if (event.type == Expose)
        {
            draw_window();
        }

        if (event.type == KeyPress)
        {
            char text[10];
            KeySym key;

            int length = XLookupString(
                &event.xkey,
                text,
                sizeof(text),
                &key,
                NULL
            );

            if (key == XK_Escape)
            {
                running = 0;
            }
            else if (key == XK_BackSpace)
            {
                if (input_length > 0)
                {
                    input_length--;
                    input[input_length] = '\0';
                }

                draw_window();
            }
            else if (key == XK_Return)
            {
                execute_command();
            }
            else if (length > 0)
            {
                if (input_length + length < INPUT_SIZE)
                {
                    memcpy(input + input_length, text, length);
                    input_length += length;
                    input[input_length] = '\0';
                }

                draw_window();
            }
        }
    }

    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}

#include <stdio.h>
#include <X11/Xlib.h>
#include "terminal.h"
#include "gui.h"
#include "input.h"
#include "scroll.h"

int running = 1;

int main(void)
{
    if (!gui_init())
        return 1;

    input[0] = '\0';

    while (running)
    {
        XEvent event;
        XNextEvent(display, &event);

        if (event.type == Expose)
        {
            gui_draw();
        }
        else if (event.type == KeyPress)
        {
            if (!handle_scroll_event(&event))
            {
                handle_keypress(&event.xkey);
            }
        }

        else if (event.type == ButtonPress)
        {
            gui_handle_button(&event.xbutton);
        }
    }

    gui_cleanup();

    return 0;
} */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <string.h>
#include <time.h>
#include "terminal.h"
#include "gui.h"
#include "input.h"
#include "scroll.h"
#include "history.h"

#define MAX_POLL_FDS (1 + MAX_TABS + MAX_TABS * MAX_WATCH_COMMANDS + MAX_TABS * MAX_JOBS)

int running = 1;

static void append_tab_output(int index, const char *text, int length)
{
    if (length <= 0)
        return;

    if (index == active_tab)
    {
        append_output(text, length);
        gui_draw();
        return;
    }

    TerminalTab *tab = &tabs[index];
    int available = OUTPUT_SIZE - 1 - tab->output_length;

    if (available <= 0)
        return;

    if (length > available)
        length = available;

    memcpy(tab->output + tab->output_length, text, length);
    tab->output_length += length;
    tab->output[tab->output_length] = '\0';
}

static void read_tab_output(int index)
{
    TerminalTab *tab = &tabs[index];
    char buffer[1024];

    if (tab->output_fd < 0)
        return;

    ssize_t n = read(tab->output_fd, buffer, sizeof(buffer));

    if (n > 0)
    {
        if (index == active_tab)
        {
            append_output(buffer, (int)n);
        }
        else
        {
            int available = OUTPUT_SIZE - 1 - tab->output_length;

            if (available > 0)
            {
                int copy = n < available ? (int)n : available;

                memcpy(tab->output + tab->output_length, buffer, copy);
                tab->output_length += copy;
                tab->output[tab->output_length] = '\0';
            }
        }

        if (index == active_tab)
            gui_draw();

        return;
    }

    if (n == 0 || (n < 0 && errno != EAGAIN && errno != EINTR))
    {
        close(tab->output_fd);
        tab->output_fd = -1;

        int finished_job_id = tab->foreground_job_id;

        int final_status = tab->last_status;
        pid_t final_pid = tab->pid;

        for (int i = 0; i < tab->child_count; i++)
        {
            int status;
            pid_t child = tab->child_pids[i];
            pid_t result;

            do
            {
                result = waitpid(child, &status, 0);
            } while (result == -1 && errno == EINTR);

            if (child == final_pid && result == child)
            {
                if (WIFEXITED(status))
                    final_status = WEXITSTATUS(status);
                else if (WIFSIGNALED(status))
                    final_status = 128 + WTERMSIG(status);
            }
        }

        tab->last_status = final_status;

        /* Mark the foreground job as completed. */
        for (int i = 0; i < MAX_JOBS; i++)
        {
            ShellJob *job = &tab->jobs[i];

            if (job->used && job->id == finished_job_id)
            {
                job->state = JOB_DONE;
                break;
            }
        }

        tab->child_count = 0;
        tab->process_running = 0;
        tab->pid = 0;
        tab->foreground_pgid = 0;
        tab->foreground_job_id = 0;

        if (index == active_tab)
            gui_draw();
    }
}

void read_job_output(int tab_index, int job_index)
{
    if (tab_index < 0 || tab_index >= tab_count ||
        job_index < 0 || job_index >= MAX_JOBS)
        return;

    TerminalTab *tab = &tabs[tab_index];
    ShellJob *job = &tab->jobs[job_index];

    if (!job->used || job->output_fd < 0)
        return;

    char buffer[1024];
    ssize_t n = read(job->output_fd, buffer, sizeof(buffer));

    if (n > 0)
    {
        append_tab_output(tab_index, buffer, (int)n);
        return;
    }

    if (n == 0 ||
        (n < 0 && errno != EAGAIN && errno != EINTR))
    {
        close(job->output_fd);
        job->output_fd = -1;

        /* Check whether every child in this job has finished. */
        int all_done = 1;

        for (int i = 0; i < job->child_count; i++)
        {
            if (!job->child_done[i])
            {
                int status;
                pid_t child = job->child_pids[i];
                pid_t result;

                do
                {
                    result = waitpid(child, &status, WNOHANG);
                } while (result == -1 && errno == EINTR);

                if (result == child)
                {
                    job->child_done[i] = 1;
                    job->child_status[i] = status;
                }
                else if (result == 0)
                {
                    all_done = 0;
                }
                else if (result == -1 && errno == ECHILD)
                {
                    /* Another handler may already have reaped it. */
                    job->child_done[i] = 1;
                }
                else
                {
                    all_done = 0;
                }
            }

            if (!job->child_done[i])
                all_done = 0;
        }

        if (all_done)
            job->state = JOB_DONE;
    }
}

static void read_watch_output(int index, int watch_index)
{
    TerminalTab *tab = &tabs[index];
    WatchCommand *watch = &tab->watch_commands[watch_index];

    if (watch->output_fd < 0)
        return;

    char buffer[1024];
    ssize_t n = read(watch->output_fd, buffer, sizeof(buffer));

    if (n > 0)
    {
        if (!watch->output_started)
        {
            char header[INPUT_SIZE + 64];

            watch->run_timestamp = time(NULL);

            int len = snprintf(
                header, sizeof(header),
                "\"%s\", %lld:\n"
                "----------------------------------------------------\n",
                watch->command,
                (long long)watch->run_timestamp);

            append_tab_output(index, header, len);
            watch->output_started = 1;
        }

        append_tab_output(index, buffer, (int)n);
        return;
    }

    if (n < 0 && (errno == EAGAIN || errno == EINTR))
        return;

    close(watch->output_fd);
    watch->output_fd = -1;

    if (watch->output_started)
    {
        const char *separator =
            "\n----------------------------------------------------\n";

        append_tab_output(index, separator, (int)strlen(separator));
    }

    for (int i = 0; i < watch->child_count; i++)
    {
        int status;
        pid_t child = watch->child_pids[i];

        while (waitpid(child, &status, 0) == -1 && errno == EINTR)
        {
        }
    }

    watch->child_count = 0;
    watch->running = 0;
    watch->output_started = 0;

    if (index == active_tab)
        gui_draw();
}

int main(void)
{

    history_init();

    fprintf(stderr, "Starting Terminator...\n");
    fflush(stderr);

    if (!gui_init())
    {
        fprintf(stderr, "gui_init() failed\n");
        return 1;
    }

    fprintf(stderr, "GUI initialized successfully\n");
    fflush(stderr);

    input[0] = '\0';

    // if (!gui_init())
    //     return 1;

    // input[0] = '\0';

    while (running)
    {

        struct pollfd fds[MAX_POLL_FDS];
        int tab_indices[MAX_POLL_FDS];
        int watch_indices[MAX_POLL_FDS];
        int job_indices[MAX_POLL_FDS];

        int count = 1;

        fds[0].fd = ConnectionNumber(display);
        fds[0].events = POLLIN;
        fds[0].revents = 0;

        tab_indices[0] = -1;
        watch_indices[0] = -1;
        job_indices[0] = -1;

        for (int i = 0; i < tab_count; i++)
        {
            if (tabs[i].process_running && tabs[i].output_fd >= 0)
            {
                fds[count].fd = tabs[i].output_fd;
                fds[count].events = POLLIN | POLLHUP | POLLERR;
                fds[count].revents = 0;

                tab_indices[count] = i;
                watch_indices[count] = -1;
                job_indices[count] = -1;

                count++;
            }
        }

        /* Add multiWatch output pipes. */
        for (int i = 0; i < tab_count; i++)
        {
            for (int j = 0; j < tabs[i].watch_command_count; j++)
            {
                WatchCommand *watch = &tabs[i].watch_commands[j];

                if (watch->running && watch->output_fd >= 0)
                {
                    fds[count].fd = watch->output_fd;
                    fds[count].events = POLLIN | POLLHUP | POLLERR;
                    fds[count].revents = 0;

                    tab_indices[count] = i;
                    watch_indices[count] = j;
                    job_indices[count] = -1;

                    count++;
                }
            }
        }

        /* Add saved job output pipes to poll(). */
        for (int i = 0; i < tab_count; i++)
        {
            for (int j = 0; j < MAX_JOBS; j++)
            {
                ShellJob *job = &tabs[i].jobs[j];

                if (job->used && job->output_fd >= 0)
                {
                    fds[count].fd = job->output_fd;
                    fds[count].events = POLLIN | POLLHUP | POLLERR;
                    fds[count].revents = 0;

                    tab_indices[count] = i;
                    watch_indices[count] = -1;
                    job_indices[count] = j;

                    count++;
                }
            }
        }
        int result = poll(fds, count, 250);

        if (result < 0)
        {
            if (errno == EINTR)
                continue;

            perror("poll");
            break;
        }

        if (fds[0].revents & (POLLIN | POLLERR | POLLHUP))
        {
            while (XPending(display))
            {
                XEvent event;
                XNextEvent(display, &event);

                if (event.type == Expose)
                {
                    gui_draw();
                }
                else if (event.type == KeyPress)
                {
                    if (!handle_scroll_event(&event))
                        handle_keypress(&event.xkey);
                }
                else if (event.type == SelectionRequest)
                {
                    handle_selection_request(&event.xselectionrequest);
                }
                else if (event.type == SelectionNotify)
                {
                    handle_selection_notify(&event.xselection);
                }
                else if (event.type == ButtonPress)
                {
                    gui_handle_button(&event.xbutton);
                }
            }
        }

        for (int i = 1; i < count; i++)
        {
            if (fds[i].revents & (POLLIN | POLLHUP | POLLERR))
            {
                if (job_indices[i] >= 0)
                {
                    read_job_output(tab_indices[i], job_indices[i]);
                }
                else if (watch_indices[i] >= 0)
                {
                    read_watch_output(tab_indices[i], watch_indices[i]);
                }
                else
                {
                    read_tab_output(tab_indices[i]);
                }
            }
        }
    }

    gui_cleanup();
    return 0;
}
