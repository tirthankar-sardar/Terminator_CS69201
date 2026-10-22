
#include <string.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include "hotkeys.h"
#include "input.h"
#include "gui.h"

extern char input[];
extern int input_length;
extern int input_cursor;

int handle_editing_hotkey(XKeyEvent *event)
{
    KeySym key = NoSymbol;
    char text[16] = {0};

    XLookupString(event, text, sizeof(text), &key, NULL);

    /* Keep the cursor within the valid input range. */
    if (input_cursor > input_length)
        input_cursor = input_length;

    if (input_cursor < 0)
        input_cursor = 0;

    /* Ctrl+A: move to the beginning of the line.
       Ctrl+E: move to the end of the line. */
    if (event->state & ControlMask)
    {
        if (key == XK_a || key == XK_A)
        {
            input_cursor = 0;
        }
        else if (key == XK_e || key == XK_E)
        {
            input_cursor = input_length;
        }
        /* Ctrl+U: delete from cursor to beginning. */
        else if (key == XK_u || key == XK_U)
        {
            memmove(input,
                    input + input_cursor,
                    (size_t)(input_length - input_cursor + 1));

            input_length -= input_cursor;
            input_cursor = 0;
        }
        /* Ctrl+K: delete from cursor to end. */
        else if (key == XK_k || key == XK_K)
        {
            input[input_cursor] = '\0';
            input_length = input_cursor;
        }
        /* Ctrl+W: delete the previous word. */
        else if (key == XK_w || key == XK_W)
        {
            int start = input_cursor;

            /* Skip spaces immediately before the cursor. */
            while (start > 0 &&
                   (input[start - 1] == ' ' ||
                    input[start - 1] == '\t'))
            {
                start--;
            }

            /* Skip the previous word. */
            while (start > 0 &&
                   input[start - 1] != ' ' &&
                   input[start - 1] != '\t')
            {
                start--;
            }

            memmove(input + start,
                    input + input_cursor,
                    (size_t)(input_length - input_cursor + 1));

            input_length -= input_cursor - start;
            input_cursor = start;
        }
        else
        {
            return 0;
        }
    }
    /* Left arrow: move cursor left. */
    else if (key == XK_Left)
    {
        if (input_cursor > 0)
            input_cursor--;
    }
    /* Right arrow: move cursor right. */
    else if (key == XK_Right)
    {
        if (input_cursor < input_length)
            input_cursor++;
    }
    else
    {
        return 0;
    }

    gui_draw();
    return 1;
}
