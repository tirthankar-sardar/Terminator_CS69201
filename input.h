
#ifndef INPUT_H
#define INPUT_H

#include <X11/Xlib.h>

void handle_keypress(XKeyEvent *event);

void handle_selection_request(
    XSelectionRequestEvent *event
);

void handle_selection_notify(
    XSelectionEvent *event
);

#endif