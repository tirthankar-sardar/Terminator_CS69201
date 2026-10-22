
#ifndef HOTKEYS_H
#define HOTKEYS_H

#include <X11/Xlib.h>

/* Handle editing shortcuts.
   Returns 1 if the key was handled, otherwise 0. */
int handle_editing_hotkey(XKeyEvent *event);

/* Current cursor position within the input buffer. */
extern int input_cursor;

#endif