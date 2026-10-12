
#ifndef VARIABLES_H
#define VARIABLES_H

#include <stddef.h>

int is_valid_name(const char *s, int len);
int handle_assignment(char *command, int tab_index);
void expand_variables(const char *src, char *dest, size_t size, int tab_index);
void clear_tab_variables(int tab_index);

#endif
