
#ifndef SHELL_H
#define SHELL_H

#include "terminal.h"

void execute_command(void);

void execute_script_line(const char *command, TerminalTab *tab);

void execute_command_string(
    const char *command,
    TerminalTab *tab);

int command_needs_more_input(const char *command);

int parse_multiwatch(
    char *command,
    char commands[][INPUT_SIZE],
    int max_commands);

void stop_multiwatch(TerminalTab *tab);

#endif
