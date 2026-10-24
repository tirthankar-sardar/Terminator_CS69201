
#ifndef COMPLETION_H
#define COMPLETION_H

#include <X11/Xlib.h>

/* Handle Tab completion for the current input line. */
void completion_handle_tab(
    char *input,
    int *cursor,
    int *length,
    const char *cwd
);

/* Handle a number or Escape while choosing a completion. */
int completion_handle_key(
    KeySym key,
    char *input,
    int *cursor,
    int *length
);

#endif
