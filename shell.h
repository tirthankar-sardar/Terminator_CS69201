// shell declaration

#ifndef SHELL_H
#define SHELL_H

void execute_command(void);
int command_needs_more_input(const char *command);
int parse_multiwatch(char *command, char commands[][INPUT_SIZE], int max_commands);
void stop_multiwatch(TerminalTab *tab);

#endif