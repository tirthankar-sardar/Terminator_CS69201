
#ifndef SCRIPT_H
#define SCRIPT_H

#include "terminal.h"

/*
 * Returns 1 if this command was recognized as a script invocation,
 * including when the script reports an error.
 *
 * Returns 0 when the command is not a script invocation.
 */
int script_handle_command(
    int argc,
    char *argv[],
    TerminalTab *tab
);

#endif
