
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "scroll.h"
#include "gui.h"

int handle_scroll_event(XEvent *event)
{
    /* X11 reports wheel up/down as button 4/5 presses. */
    if (event->type == ButtonPress)
    {
        if (event->xbutton.button == Button4)
        {
            gui_scroll(3);
            return 1;
        }
        else if (event->xbutton.button == Button5)
        {
            gui_scroll(-3);
            return 1;
        }

        return 0;
    }

    if (event->type != KeyPress)
        return 0;

    KeySym key = XLookupKeysym(&event->xkey, 0);

    if (key == XK_Up)
    {
        gui_scroll(3);
        return 1;
    }
    else if (key == XK_Down)
    {
        gui_scroll(-3);
        return 1;
    }

    return 0;
}
