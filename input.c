// keyboard handling

#include <string.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include "terminal.h"
#include "input.h"
#include "gui.h"
#include "shell.h"

char input[INPUT_SIZE];
int input_length = 0;

void handle_keypress(XKeyEvent *event)
{
    char text[10];
    KeySym key;

    int length = XLookupString(
        event,
        text,
        sizeof(text),
        &key,
        NULL);

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

        gui_draw();
    }
    // else if (key == XK_Return || key == XK_KP_Enter)
    // {
    //     execute_command();
    //     input_length = 0;
    //     input[0] = '\0';

    //     gui_draw();
    // }

    else if (key == XK_Return || key == XK_KP_Enter)
    {
        input[input_length] = '\0';

        if (command_needs_more_input(input))
        {
            if (input_length + 1 < INPUT_SIZE)
            {
                input[input_length++] = '\n';
                input[input_length] = '\0';

                tabs[active_tab].continuation_mode = 1;
            }
        }
        else
        {
            tabs[active_tab].continuation_mode = 0;
            execute_command();
        }

        gui_draw();
    }

    else if (length > 0)
    {
        if (input_length + length < INPUT_SIZE)
        {
            memcpy(input + input_length, text, length);
            input_length += length;
            input[input_length] = '\0';
        }

        gui_draw();
    }
}