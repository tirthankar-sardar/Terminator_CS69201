// // keyboard handling

// #include <string.h>
// #include <stdio.h>
// #include <X11/Xutil.h>
// #include <X11/keysym.h>
// #include <signal.h>
// #include <unistd.h>
// #include "hotkeys.h"
// #include "terminal.h"
// #include "input.h"
// #include "gui.h"
// #include "shell.h"

// char input[INPUT_SIZE];
// int input_length = 0;

// void handle_keypress(XKeyEvent *event)
// {
//     char text[16] = {0};
//     KeySym key = NoSymbol;

//     int length = XLookupString(
//         event, text, sizeof(text), &key, NULL);

//     KeySym raw_key = XLookupKeysym(event, 0);

//     if ((event->state & ControlMask) && raw_key == XK_c)
//     {
//         TerminalTab *tab = &tabs[active_tab];

//         if (tab->multiwatch_active)
//         {
//             stop_multiwatch(tab);
//         }
//         else if (tab->process_running && tab->foreground_pgid > 0)
//         {
//             if (kill(-tab->foreground_pgid, SIGINT) == -1)
//                 perror("kill SIGINT");
//         }

//         return;
//     }

//     if (handle_editing_hotkey(event))
//         return;
//     else if (key == XK_Escape)
//     {
//         running = 0;
//     }
//     else if (key == XK_BackSpace)
//     {
//         if (input_cursor > 0)
//         {
//             memmove(input + input_cursor - 1,
//                     input + input_cursor,
//                     input_length - input_cursor + 1);

//             input_cursor--;
//             input_length--;
//         }
//         gui_draw();
//     }
//     // else if (key == XK_Return || key == XK_KP_Enter)
//     // {
//     //     execute_command();
//     //     input_length = 0;
//     //     input[0] = '\0';

//     //     gui_draw();
//     // }

//     else if (key == XK_Return || key == XK_KP_Enter)
//     {
//         input[input_length] = '\0';

//         int backslashes = 0;

//         for (int i = input_length - 1;
//              i >= 0 && input[i] == '\\';
//              i--)
//         {
//             backslashes++;
//         }

//         int trailing_backslash = (backslashes % 2 == 1);

//         if (trailing_backslash || command_needs_more_input(input))
//         {
//             if (input_length + 1 < INPUT_SIZE)
//             {
//                 input[input_length++] = '\n';
//                 input[input_length] = '\0';
//                 tabs[active_tab].continuation_mode = 1;
//             }
//         }
//         else
//         {
//             tabs[active_tab].continuation_mode = 0;
//             execute_command();
//             input_cursor = 0;
//         }

//         gui_draw();
//     }

//     else if (length > 0)
//     {
//         if (input_length + length < INPUT_SIZE)
//         {
//             memmove(input + input_cursor + length,
//                     input + input_cursor,
//                     input_length - input_cursor + 1);

//             memcpy(input + input_cursor, text, length);

//             input_cursor += length;
//             input_length += length;
//         }

//         gui_draw();
//     }
// }

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "hotkeys.h"
#include "terminal.h"
#include "input.h"
#include "gui.h"
#include "shell.h"
#include "history.h"

static int history_search_mode = 0;
static char history_search_term[INPUT_SIZE];
static int history_search_length = 0;

int history_search_active(void)
{
    return history_search_mode;
}

const char *history_search_text(void)
{
    return history_search_term;
}

extern int running;

char input[INPUT_SIZE];
int input_length = 0;
int input_cursor = 0;

#define HISTORY_LIMIT 100

static char history[MAX_TABS][HISTORY_LIMIT][INPUT_SIZE];
static int history_count[MAX_TABS];
static int history_pos[MAX_TABS];
static char history_scratch[MAX_TABS][INPUT_SIZE];

/* Replace the current input line. */
static void set_input(const char *s)
{
    if (!s)
        s = "";

    snprintf(input, INPUT_SIZE, "%s", s);
    input_length = (int)strlen(input);
    input_cursor = input_length;
}

/* Save a successfully submitted command. */
static void remember_command(int tab_index, const char *command)
{
    if (!command || !*command)
        return;

    int n = history_count[tab_index];

    /* Avoid storing consecutive duplicates. */
    if (n > 0 &&
        strcmp(history[tab_index][n - 1], command) == 0)
    {
        history_pos[tab_index] = history_count[tab_index];
        return;
    }

    /* Remove the oldest entry when history is full. */
    if (n == HISTORY_LIMIT)
    {
        memmove(history[tab_index],
                history[tab_index] + 1,
                sizeof(history[tab_index][0]) *
                    (HISTORY_LIMIT - 1));

        n--;
        history_count[tab_index] = n;
    }

    snprintf(history[tab_index][n],
             INPUT_SIZE, "%s", command);

    history_count[tab_index]++;
    history_pos[tab_index] = history_count[tab_index];
}

/* Browse previously entered commands using Up and Down. */
static void browse_history(int direction)
{
    int t = active_tab;

    if (direction < 0)
    {
        if (history_count[t] == 0 || history_pos[t] <= 0)
            return;

        if (history_pos[t] == history_count[t])
        {
            snprintf(history_scratch[t], INPUT_SIZE,
                     "%s", input);
        }

        history_pos[t]--;
        set_input(history[t][history_pos[t]]);
    }
    else
    {
        if (history_pos[t] >= history_count[t])
            return;

        history_pos[t]++;

        if (history_pos[t] == history_count[t])
            set_input(history_scratch[t]);
        else
            set_input(history[t][history_pos[t]]);
    }

    gui_draw();
}

/* X11 clipboard and selection state. */
static char clipboard_text[INPUT_SIZE];

static Atom clipboard_atom = None;
static Atom utf8_atom = None;
static Atom targets_atom = None;
static Atom paste_property = None;

static void init_clipboard_atoms(void)
{
    if (clipboard_atom == None)
        clipboard_atom =
            XInternAtom(display, "CLIPBOARD", False);

    if (utf8_atom == None)
        utf8_atom =
            XInternAtom(display, "UTF8_STRING", False);

    if (targets_atom == None)
        targets_atom =
            XInternAtom(display, "TARGETS", False);

    if (paste_property == None)
        paste_property =
            XInternAtom(display, "TERMINATOR_PASTE", False);
}

/* Copy the current command line to X11 selections. */
static void copy_input_to_selection(void)
{
    init_clipboard_atoms();

    snprintf(clipboard_text, sizeof(clipboard_text),
             "%s", input);

    XSetSelectionOwner(display, clipboard_atom,
                       window, CurrentTime);

    XSetSelectionOwner(display, XA_PRIMARY,
                       window, CurrentTime);

    XFlush(display);
}

/* Ask the X11 clipboard owner to send its contents. */
static void request_paste(void)
{
    init_clipboard_atoms();

    XConvertSelection(display,
                      clipboard_atom,
                      utf8_atom,
                      paste_property,
                      window,
                      CurrentTime);

    XFlush(display);
}

/* Respond when another application requests our selection. */
void handle_selection_request(XSelectionRequestEvent *request)
{
    init_clipboard_atoms();

    XSelectionEvent reply;
    memset(&reply, 0, sizeof(reply));

    reply.type = SelectionNotify;
    reply.display = request->display;
    reply.requestor = request->requestor;
    reply.selection = request->selection;
    reply.target = request->target;
    reply.time = request->time;
    reply.property = None;

    Atom property =
        request->property == None
            ? request->target
            : request->property;

    if (request->target == targets_atom)
    {
        Atom targets[] = {
            targets_atom,
            XA_STRING,
            utf8_atom};

        XChangeProperty(display,
                        request->requestor,
                        property,
                        XA_ATOM,
                        32,
                        PropModeReplace,
                        (unsigned char *)targets,
                        3);

        reply.property = property;
    }
    else if (request->target == XA_STRING ||
             request->target == utf8_atom)
    {
        XChangeProperty(display,
                        request->requestor,
                        property,
                        request->target,
                        8,
                        PropModeReplace,
                        (unsigned char *)clipboard_text,
                        (int)strlen(clipboard_text));

        reply.property = property;
    }

    XSendEvent(display, request->requestor,
               False, 0, (XEvent *)&reply);

    XFlush(display);
}

/* Insert the clipboard text when the paste request completes. */
void handle_selection_notify(XSelectionEvent *event)
{
    init_clipboard_atoms();

    if (event->selection != clipboard_atom ||
        event->property == None)
    {
        return;
    }

    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;

    if (XGetWindowProperty(display,
                           window,
                           paste_property,
                           0,
                           INPUT_SIZE,
                           True,
                           AnyPropertyType,
                           &actual_type,
                           &actual_format,
                           &nitems,
                           &bytes_after,
                           &data) != Success ||
        !data)
    {
        return;
    }

    if (actual_format == 8 &&
        nitems > 0 &&
        input_length + (int)nitems < INPUT_SIZE)
    {
        memmove(input + input_cursor + nitems,
                input + input_cursor,
                (size_t)(input_length - input_cursor + 1));

        memcpy(input + input_cursor, data, nitems);

        input_cursor += (int)nitems;
        input_length += (int)nitems;

        gui_draw();
    }

    XFree(data);
}

static void start_history_search(void)
{
    history_search_mode = 1;
    history_search_length = 0;
    history_search_term[0] = '\0';

    append_output("\nReverse history search (Ctrl+R)\n",
                  (int)strlen("\nReverse history search (Ctrl+R)\n"));

    append_output("Enter search term: ",
                  (int)strlen("Enter search term: "));

    gui_draw();
}

/* Process keyboard input and hotkeys. */
void handle_keypress(XKeyEvent *event)
{
    char text[16] = {0};
    KeySym key = NoSymbol;

    int length = XLookupString(
        event, text, (int)sizeof(text), &key, NULL);

    KeySym raw_key = XLookupKeysym(event, 0);

    TerminalTab *tab = &tabs[active_tab];

    /* Handle keyboard input while Ctrl+R search is active. */
    if (history_search_mode)
    {
        /* Escape cancels the search. */
        if (key == XK_BackSpace || raw_key == XK_BackSpace)
        {
            if (history_search_length > 0)
            {
                history_search_length--;
                history_search_term[history_search_length] = '\0';
            }

            gui_draw();
            return;
        }

        /* Backspace removes one search character. */
        if (key == XK_BackSpace)
        {
            if (history_search_length > 0)
            {
                history_search_length--;
                history_search_term[history_search_length] = '\0';
            }

            gui_draw();
            return;
        }

        /* Enter completes the search. */
        if (key == XK_Return || key == XK_KP_Enter)
        {
            history_search_mode = 0;

            append_output("\n", 1);
            history_search(history_search_term);

            history_search_term[0] = '\0';
            history_search_length = 0;

            gui_draw();
            return;
        }

        /* Insert printable characters into the search term. */
        if (length > 0 &&
            !(event->state & (ControlMask | Mod1Mask)))
        {
            if (history_search_length + length < INPUT_SIZE)
            {
                memcpy(history_search_term + history_search_length,
                       text, (size_t)length);

                history_search_length += length;
                history_search_term[history_search_length] = '\0';
            }

            gui_draw();
        }

        return;
    }

    /* Ctrl+Shift+C: copy before checking Ctrl+C. */
    if ((event->state & ControlMask) &&
        (event->state & ShiftMask) &&
        (raw_key == XK_c || raw_key == XK_C))
    {
        copy_input_to_selection();
        return;
    }

    /* Ctrl+Shift+V: paste. */
    if ((event->state & ControlMask) &&
        (event->state & ShiftMask) &&
        (raw_key == XK_v || raw_key == XK_V))
    {
        request_paste();
        return;
    }

    /* Ctrl+R: start searchable history. */
    if ((event->state & ControlMask) &&
        !(event->state & ShiftMask) &&
        (raw_key == XK_r || raw_key == XK_R))
    {
        start_history_search();
        return;
    }

    /*
     * Ctrl+C:
     * - Stop multiwatch if active.
     * - Interrupt the foreground process if one is running.
     * - Otherwise cancel the current input line.
     */
    if ((event->state & ControlMask) &&
        !(event->state & ShiftMask) &&
        (raw_key == XK_c || raw_key == XK_C))
    {
        if (tab->multiwatch_active)
        {
            stop_multiwatch(tab);
        }
        // else if (tab->process_running &&
        //          tab->foreground_pgid > 0)
        // {
        //     if (kill(-tab->foreground_pgid, SIGINT) == -1)
        //         perror("kill(SIGINT)");
        // }
        else if (tab->process_running &&
                 tab->foreground_pgid > 0)
        {
            if (kill(-tab->foreground_pgid, SIGINT) == -1)
            {
                perror("kill(SIGINT)");
            }
            else
            {
                append_output("^C\n", 3);
                gui_draw();
            }
        }
        else
        {
            input[0] = '\0';
            input_length = 0;
            input_cursor = 0;

            tab->continuation_mode = 0;

            append_output("^C\n", 3);
            gui_draw();
        }

        return;
    }

    /* Ctrl+Z : Suspend the foreground process. */
    // if ((event->state & ControlMask) &&
    //     !(event->state & ShiftMask) &&
    //     (raw_key == XK_z || raw_key == XK_Z))
    // {
    //     if (tab->process_running &&
    //         tab->foreground_pgid > 0)
    //     {
    //         job_control_key(1);
    //     }

    //     return;
    // }

    /* Ctrl+Z: suspend the foreground job. */
    if ((event->state & ControlMask) &&
        !(event->state & ShiftMask) &&
        (raw_key == XK_z || raw_key == XK_Z))
    {
        job_control_key(1);
        return;
    }

    /* Ctrl+A, Ctrl+E, Ctrl+U, Ctrl+K, Ctrl+W, arrows. */
    if (handle_editing_hotkey(event))
        return;

    /* Up and Down: command history. */
    if (raw_key == XK_Up)
    {
        browse_history(-1);
        return;
    }

    if (raw_key == XK_Down)
    {
        browse_history(1);
        return;
    }

    /* Escape: close the application. */
    // if (raw_key == XK_Escape)
    // {
    //     running = 0;
    //     return;
    // }

    /* Escape: cancel the current operation without closing Terminator. */
    if (raw_key == XK_Escape)
    {
        return;
    }

    /* Ctrl+L: clear the visible terminal output. */
    if ((event->state & ControlMask) &&
        (raw_key == XK_l || raw_key == XK_L))
    {
        /* Clear the active terminal's displayed output. */
        output[0] = '\0';
        output_length = 0;

        /* Keep the active tab's stored output in sync. */
        tab->output[0] = '\0';
        tab->output_length = 0;

        gui_draw();
        return;
    }

    /* Ctrl+D: delete at cursor, or exit at an empty prompt. */
    if ((event->state & ControlMask) &&
        (raw_key == XK_d || raw_key == XK_D))
    {
        if (input_length == 0)
        {
            running = 0;
        }
        else if (input_cursor < input_length)
        {
            memmove(input + input_cursor,
                    input + input_cursor + 1,
                    (size_t)(input_length - input_cursor));

            input_length--;
            gui_draw();
        }

        return;
    }

    /* Backspace: delete the character before the cursor. */
    if (key == XK_BackSpace)
    {
        if (input_cursor > 0)
        {
            memmove(input + input_cursor - 1,
                    input + input_cursor,
                    (size_t)(input_length - input_cursor + 1));

            input_cursor--;
            input_length--;
        }

        gui_draw();
        return;
    }

    /* Delete: remove the character at the cursor. */
    if (key == XK_Delete)
    {
        if (input_cursor < input_length)
        {
            memmove(input + input_cursor,
                    input + input_cursor + 1,
                    (size_t)(input_length - input_cursor));

            input_length--;
        }

        gui_draw();
        return;
    }

    /* Enter: submit the command or continue a multiline command. */
    if (key == XK_Return || key == XK_KP_Enter)
    {
        input[input_length] = '\0';

        int backslashes = 0;

        for (int i = input_length - 1;
             i >= 0 && input[i] == '\\';
             i--)
        {
            backslashes++;
        }

        int trailing_backslash = (backslashes % 2 == 1);

        if (trailing_backslash ||
            command_needs_more_input(input))
        {
            if (input_length + 1 < INPUT_SIZE)
            {
                input[input_length++] = '\n';
                input[input_length] = '\0';
                input_cursor = input_length;

                tab->continuation_mode = 1;
            }
        }
        else
        {
            remember_command(active_tab, input);

            tab->continuation_mode = 0;

            history_add(input);

            execute_command();

            input_cursor = 0;
            history_pos[active_tab] =
                history_count[active_tab];
        }

        gui_draw();
        return;
    }

    /* Insert ordinary printable characters at the cursor. */
    if (length > 0 &&
        !(event->state & (ControlMask | Mod1Mask)))
    {
        if (input_length + length < INPUT_SIZE)
        {
            memmove(input + input_cursor + length,
                    input + input_cursor,
                    (size_t)(input_length - input_cursor + 1));

            memcpy(input + input_cursor,
                   text,
                   (size_t)length);

            input_cursor += length;
            input_length += length;
        }

        gui_draw();
    }
}
