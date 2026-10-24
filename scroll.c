
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "scroll.h"
#include "gui.h"

int handle_scroll_event(XEvent *event)
{
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