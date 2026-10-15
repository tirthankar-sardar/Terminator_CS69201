
// window creation and drawing functions

/*#include <string.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include "terminal.h"
#include "gui.h"

Display *display;
Window window;
GC gc;

char output[OUTPUT_SIZE];
int output_length = 0;
static int scroll_offset = 0;

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
    scroll_offset = 0;
}

int gui_init(void)
{
    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        fprintf(stderr, "Cannot open X display\n");
        return 0;
    }

    int screen = DefaultScreen(display);
    window = XCreateSimpleWindow(display, RootWindow(display, screen),
                                 100, 100, 700, 450, 1,
                                 BlackPixel(display, screen),
                                 WhitePixel(display, screen));
    XStoreName(display, window, "Terminator");
    XSelectInput(display, window, ExposureMask | KeyPressMask);
    gc = DefaultGC(display, screen);
    XMapWindow(display, window);
    output[0] = '\0';
    return 1;
}

void gui_draw(void)
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
                XDrawString(display, window, gc, x, y, output + start, length);
            y += line_height;
            start = i + 1;
        }
    }

    if (y > 385)
        y = 385;
    XDrawString(display, window, gc, x, y, "$ ", 2);
    XDrawString(display, window, gc, x + 20, y, input, input_length);
    XFlush(display);
}
// this fucntion is without scroll, so we will implement a new function with scroll

void gui_draw(void)
{
    XWindowAttributes attributes;
    XGetWindowAttributes(display, window, &attributes);
    XClearWindow(display, window);

    int x = 15;
    int line_height = 18;
    int first_y = 25;
    int prompt_y = attributes.height - 15;
    int visible_lines = (prompt_y - first_y) / line_height;
    if (visible_lines < 0)
        visible_lines = 0;

    static int line_starts[OUTPUT_SIZE + 1];
    int line_count = 1;
    line_starts[0] = 0;
    for (int i = 0; i < output_length; i++)
    {
        if (output[i] == '\n')
        {
            line_starts[line_count] = i + 1;
            line_count++;
        }
    }

    int end_line = line_count - scroll_offset;
    if (end_line < 0)
        end_line = 0;
    if (end_line > line_count)
        end_line = line_count;

    int start_line = end_line - visible_lines;
    if (start_line < 0)
        start_line = 0;

    int row = 0;
    for (int i = start_line; i < end_line; i++)
    {
        int start = line_starts[i];
        int end = i + 1 < line_count ? line_starts[i + 1] - 1 : output_length;
        int length = end - start;
        if (length > 0)
            XDrawString(display, window, gc, x,
                        first_y + row * line_height, output + start, length);
        row++;
    }

    XDrawString(display, window, gc, x, prompt_y, "$ ", 2);
    XDrawString(display, window, gc, x + 20, prompt_y, input, input_length);
    XFlush(display);
}

void gui_scroll(int lines)
{
    scroll_offset += lines;
    if (scroll_offset < 0)
        scroll_offset = 0;
    if (scroll_offset > output_length)
        scroll_offset = output_length;
    gui_draw();
}

void gui_cleanup(void)
{
    if (display != NULL)
    {
        XDestroyWindow(display, window);
        XCloseDisplay(display);
    }
}*/

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include "terminal.h"
#include "gui.h"

Display *display;
Window window;
GC gc;

char output[OUTPUT_SIZE];
int output_length = 0;

TerminalTab tabs[MAX_TABS];
int tab_count = 1;
int active_tab = 0;

#define TAB_HEIGHT 30
#define TAB_WIDTH 100

static void save_active_tab(void)
{
    if (active_tab < 0 || active_tab >= tab_count)
        return;

    TerminalTab *tab = &tabs[active_tab];

    memcpy(tab->input, input, INPUT_SIZE);
    tab->input_length = input_length;

    memcpy(tab->output, output, OUTPUT_SIZE);
    tab->output_length = output_length;
}

static void load_active_tab(void)
{
    if (active_tab < 0 || active_tab >= tab_count)
        return;

    TerminalTab *tab = &tabs[active_tab];

    memcpy(input, tab->input, INPUT_SIZE);
    input_length = tab->input_length;

    memcpy(output, tab->output, OUTPUT_SIZE);
    output_length = tab->output_length;
}

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

    tabs[active_tab].scroll_offset = 0;
}

int gui_init(void)
{
    display = XOpenDisplay(NULL);

    if (display == NULL)
    {
        fprintf(stderr, "Cannot open X display\n");
        return 0;
    }

    int screen = DefaultScreen(display);

    window = XCreateSimpleWindow(
        display,
        RootWindow(display, screen),
        100, 100, 700, 450, 1,
        BlackPixel(display, screen),
        WhitePixel(display, screen));

    XStoreName(display, window, "Terminator");

    XSelectInput(
        display,
        window,
        ExposureMask | KeyPressMask | ButtonPressMask);

    gc = DefaultGC(display, screen);

    memset(tabs, 0, sizeof(tabs));
    memset(input, 0, INPUT_SIZE);
    memset(output, 0, OUTPUT_SIZE);

    tabs[0].pid = 0;
    tabs[0].output_fd = -1;
    tabs[0].process_running = 0;
    tabs[0].last_status = 0;

    input_length = 0;
    output_length = 0;
    tab_count = 1;
    active_tab = 0;

    if (getcwd(tabs[0].cwd, sizeof(tabs[0].cwd)) == NULL)
    snprintf(tabs[0].cwd, sizeof(tabs[0].cwd), "/");

    XMapWindow(display, window);
    XFlush(display);
    return 1;
}

void gui_draw(void)
{
    if (display == NULL)
        return;

    save_active_tab();

    XWindowAttributes attributes;
    XGetWindowAttributes(display, window, &attributes);

    XClearWindow(display, window);

    /* Draw tab bar */
    for (int i = 0; i < tab_count; i++)
    {
        int x = i * TAB_WIDTH;

        XDrawRectangle(
            display, window, gc,
            x, 0, TAB_WIDTH - 1, TAB_HEIGHT);

        char label[32];
        snprintf(label, sizeof(label), "Tab %d", i + 1);

        XDrawString(
            display, window, gc,
            x + 10, 20,
            label, (int)strlen(label));

        if (i == active_tab)
        {
            XDrawLine(
                display, window, gc,
                x + 2, TAB_HEIGHT - 2,
                x + TAB_WIDTH - 3, TAB_HEIGHT - 2);
        }
    }

    if (tab_count < MAX_TABS)
    {
        int x = tab_count * TAB_WIDTH;

        XDrawRectangle(
            display, window, gc,
            x, 0, TAB_WIDTH - 1, TAB_HEIGHT);

        XDrawString(display, window, gc, x + 10, 20, "+", 1);
    }

    /* Draw terminal output */
    int x = 15;
    int line_height = 18;
    int first_y = TAB_HEIGHT + 20;
    int prompt_y = attributes.height - 15;

    int visible_lines = (prompt_y - first_y) / line_height;

    if (visible_lines < 0)
        visible_lines = 0;

    static int line_starts[OUTPUT_SIZE + 1];

    int line_count = 1;
    line_starts[0] = 0;

    for (int i = 0; i < output_length; i++)
    {
        if (output[i] == '\n')
        {
            line_starts[line_count] = i + 1;
            line_count++;
        }
    }

    int end_line = line_count - tabs[active_tab].scroll_offset;

    if (end_line < 0)
        end_line = 0;

    if (end_line > line_count)
        end_line = line_count;

    int start_line = end_line - visible_lines;

    if (start_line < 0)
        start_line = 0;

    int row = 0;

    for (int i = start_line; i < end_line; i++)
    {
        int start = line_starts[i];
        int end = (i + 1 < line_count)
                      ? line_starts[i + 1] - 1
                      : output_length;

        int length = end - start;

        if (length > 0)
        {
            XDrawString(
                display, window, gc,
                x, first_y + row * line_height,
                output + start, length);
        }

        row++;
    }

    /* Draw prompt and input */
    XDrawString(display, window, gc, x, prompt_y, "$ ", 2);
    XDrawString(
        display, window, gc,
        x + 20, prompt_y,
        input, input_length);

    XFlush(display);
}

void gui_scroll(int lines)
{
    tabs[active_tab].scroll_offset += lines;

    if (tabs[active_tab].scroll_offset < 0)
        tabs[active_tab].scroll_offset = 0;

    if (tabs[active_tab].scroll_offset > output_length)
        tabs[active_tab].scroll_offset = output_length;

    gui_draw();
}

void gui_switch_tab(int index)
{
    if (index < 0 || index >= tab_count || index == active_tab)
        return;

    save_active_tab();
    active_tab = index;
    load_active_tab();
    gui_draw();
}

/*void gui_new_tab(void)
{
    if (tab_count >= MAX_TABS)
        return;

    save_active_tab();

    active_tab = tab_count;
    memset(&tabs[active_tab], 0, sizeof(TerminalTab));
    tabs[tab_count] = (TerminalTab){0};

    tabs[tab_count].pid = 0;
    tabs[tab_count].output_fd = -1;
    tabs[tab_count].process_running = 0;
    tabs[tab_count].last_status = 0;

    tab_count++;

    load_active_tab();
    gui_draw();
}*/

void gui_new_tab(void)
{
    if (tab_count >= MAX_TABS)
        return;

    char initial_cwd[PATH_MAX];

    snprintf(initial_cwd, sizeof(initial_cwd), "%s",
             tabs[active_tab].cwd);

    save_active_tab();

    active_tab = tab_count;
    memset(&tabs[active_tab], 0, sizeof(TerminalTab));

    snprintf(tabs[active_tab].cwd, sizeof(tabs[active_tab].cwd),
             "%s", initial_cwd);

    tabs[active_tab].pid = 0;
    tabs[active_tab].output_fd = -1;
    tabs[active_tab].process_running = 0;
    tabs[active_tab].last_status = 0;

    tab_count++;

    load_active_tab();
    gui_draw();
}


void gui_close_tab(void)
{
    if (tab_count <= 1)
    {
        running = 0;
        return;
    }

    save_active_tab();

    for (int i = active_tab; i < tab_count - 1; i++)
        tabs[i] = tabs[i + 1];

    tab_count--;
    getcwd(tabs[active_tab].cwd, sizeof(tabs[active_tab].cwd));

    if (active_tab >= tab_count)
        active_tab = tab_count - 1;

    load_active_tab();
    gui_draw();
}

void gui_handle_button(XButtonEvent *event)
{
    if (event->y < TAB_HEIGHT)
    {
        int index = event->x / TAB_WIDTH;

        if (index >= 0 && index < tab_count)
            gui_switch_tab(index);
        else if (index == tab_count)
            gui_new_tab();
    }
}

void gui_cleanup(void)
{
    if (display != NULL)
    {
        XDestroyWindow(display, window);
        XCloseDisplay(display);
        display = NULL;
    }
}
