#include <stdio.h>
#include <X11/Xlib.h>

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

    XStoreName(display, window, "Hello World");

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
                "Hello World",
                11);
        }

        if (event.type == KeyPress)
        {
            break;
        }
    }

    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}