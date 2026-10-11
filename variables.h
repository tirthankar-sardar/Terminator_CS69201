#ifndef VARIABLES_H
#define VARIABLES_H

#include <stddef.h>

int is_valid_name(const char *s, int len);
int handle_assignment(char *command);
void expand_variables(const char *src, char *dest, size_t size);

#endif