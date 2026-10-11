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
} */

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
}