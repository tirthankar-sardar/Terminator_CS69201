
/*#include "variables.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

int is_valid_name(const char *s, int len)
{
    if (len == 0 || !(isalpha((unsigned char)s[0]) || s[0] == '_'))
        return 0;

    for (int i = 1; i < len; i++)
    {
        if (!(isalnum((unsigned char)s[i]) || s[i] == '_'))
            return 0;
    }

    return 1;
}

int handle_assignment(char *command)
{
    char *eq = strchr(command, '=');

    if (eq == NULL)
        return 0;

    int len = (int)(eq - command);

    if (!is_valid_name(command, len))
        return 0;

    char name[256];

    if ((size_t)len >= sizeof(name))
        return 0;

    memcpy(name, command, len);
    name[len] = '\0';

    char *value = eq + 1;
    size_t value_len = strlen(value);

    if (value_len >= 2 &&
        ((value[0] == '"' && value[value_len - 1] == '"') ||
         (value[0] == '\'' && value[value_len - 1] == '\'')))
    {
        value[value_len - 1] = '\0';
        value++;
    }

    if (setenv(name, value, 1) != 0)
        return 0;

    return 1;
}

void expand_variables(const char *src, char *dest, size_t size)
{
    size_t i = 0, j = 0;
    char quote = '\0';

    while (src[i] != '\0' && j + 1 < size)
    {
        if (src[i] == '\'' && quote != '"')
        {
            quote = (quote == '\'') ? '\0' : '\'';
            dest[j++] = src[i++];
            continue;
        }

        if (src[i] == '"' && quote != '\'')
        {
            quote = (quote == '"') ? '\0' : '"';
            dest[j++] = src[i++];
            continue;
        }

        if (src[i] == '$' && quote != '\'')
        {
            size_t start = i + 1;
            size_t end = start;

            if (src[start] == '{')
            {
                start++;
                end = start;

                while (src[end] != '\0' && src[end] != '}')
                    end++;

                if (src[end] != '}')
                {
                    dest[j++] = src[i++];
                    continue;
                }

                i = end + 1;
            }
            else
            {
                if (!(isalpha((unsigned char)src[start]) ||
                      src[start] == '_'))
                {
                    dest[j++] = src[i++];
                    continue;
                }

                while (isalnum((unsigned char)src[end]) ||
                       src[end] == '_')
                    end++;

                i = end;
            }

            char name[256];
            size_t len = end - start;

            if (len >= sizeof(name))
                len = sizeof(name) - 1;

            memcpy(name, src + start, len);
            name[len] = '\0';

            const char *value = getenv(name);

            if (value != NULL)
            {
                while (*value && j + 1 < size)
                    dest[j++] = *value++;
            }

            continue;
        }

        dest[j++] = src[i++];
    }

    dest[j] = '\0';
}*/

#include "variables.h"
#include "terminal.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VARIABLES 128
#define VARIABLE_NAME_SIZE 256
#define VARIABLE_VALUE_SIZE 1024

typedef struct
{
    char name[VARIABLE_NAME_SIZE];
    char value[VARIABLE_VALUE_SIZE];
    int used;
} ShellVariable;

static ShellVariable variables[MAX_TABS][MAX_VARIABLES];

int is_valid_name(const char *s, int len)
{
    if (len <= 0 ||
        !(isalpha((unsigned char)s[0]) || s[0] == '_'))
        return 0;

    for (int i = 1; i < len; i++)
    {
        if (!(isalnum((unsigned char)s[i]) || s[i] == '_'))
            return 0;
    }

    return 1;
}

static const char *get_tab_variable(const char *name, int tab_index)
{
    if (tab_index < 0 || tab_index >= MAX_TABS)
        return NULL;

    for (int i = 0; i < MAX_VARIABLES; i++)
    {
        if (variables[tab_index][i].used &&
            strcmp(variables[tab_index][i].name, name) == 0)
        {
            return variables[tab_index][i].value;
        }
    }

    return NULL;
}

static int set_tab_variable(const char *name, const char *value,
                            int tab_index)
{
    if (tab_index < 0 || tab_index >= MAX_TABS)
        return 0;

    size_t name_len = strlen(name);
    size_t value_len = strlen(value);

    if (name_len >= VARIABLE_NAME_SIZE ||
        value_len >= VARIABLE_VALUE_SIZE)
        return 0;

    int free_slot = -1;

    for (int i = 0; i < MAX_VARIABLES; i++)
    {
        if (variables[tab_index][i].used &&
            strcmp(variables[tab_index][i].name, name) == 0)
        {
            free_slot = i;
            break;
        }

        if (!variables[tab_index][i].used && free_slot == -1)
            free_slot = i;
    }

    if (free_slot == -1)
        return 0;

    strcpy(variables[tab_index][free_slot].name, name);
    strcpy(variables[tab_index][free_slot].value, value);
    variables[tab_index][free_slot].used = 1;

    return 1;
}

int handle_assignment(char *command, int tab_index)
{
    char *eq = strchr(command, '=');

    if (eq == NULL)
        return 0;

    int len = (int)(eq - command);

    if (!is_valid_name(command, len))
        return 0;

    char name[VARIABLE_NAME_SIZE];

    if (len >= VARIABLE_NAME_SIZE)
        return 0;

    memcpy(name, command, len);
    name[len] = '\0';

    char *value = eq + 1;
    size_t value_len = strlen(value);

    if (value_len >= 2 &&
        ((value[0] == '"' && value[value_len - 1] == '"') ||
         (value[0] == '\'' && value[value_len - 1] == '\'')))
    {
        value[value_len - 1] = '\0';
        value++;
    }

    if (!set_tab_variable(name, value, tab_index))
        return 0;

    return 1;
}

void expand_variables(const char *src, char *dest, size_t size,
                      int tab_index)
{
    size_t i = 0;
    size_t j = 0;
    char quote = '\0';

    if (size == 0)
        return;

    while (src[i] != '\0' && j + 1 < size)
    {
        if (src[i] == '\'' && quote != '"')
        {
            quote = (quote == '\'') ? '\0' : '\'';
            dest[j++] = src[i++];
            continue;
        }

        if (src[i] == '"' && quote != '\'')
        {
            quote = (quote == '"') ? '\0' : '"';
            dest[j++] = src[i++];
            continue;
        }

        if (src[i] != '$' || quote == '\'')
        {
            dest[j++] = src[i++];
            continue;
        }

        /* Expand $? to the previous command's exit status. */
        if (src[i] == '$' && src[i + 1] == '?')
        {
            char status_text[16];
            int status = 0;

            if (tab_index >= 0 && tab_index < MAX_TABS)
                status = tabs[tab_index].last_status;

            snprintf(status_text, sizeof(status_text), "%d", status);

            for (size_t k = 0;
                 status_text[k] != '\0' && j + 1 < size;
                 k++)
            {
                dest[j++] = status_text[k];
            }

            i += 2;
            continue;
        }

        size_t dollar = i;
        size_t start = i + 1;
        size_t end = start;
        int braced = 0;

        if (src[start] == '{')
        {
            braced = 1;
            start++;

            end = start;
            while (src[end] != '\0' && src[end] != '}')
                end++;

            if (src[end] != '}')
            {
                dest[j++] = src[i++];
                continue;
            }
        }
        else
        {
            if (!(isalpha((unsigned char)src[start]) ||
                  src[start] == '_'))
            {
                dest[j++] = src[i++];
                continue;
            }

            while (isalnum((unsigned char)src[end]) ||
                   src[end] == '_')
                end++;
        }

        size_t name_len = end - start;

        if (name_len == 0 || name_len >= VARIABLE_NAME_SIZE)
        {
            dest[j++] = src[dollar++];
            i = dollar;
            continue;
        }

        char name[VARIABLE_NAME_SIZE];
        memcpy(name, src + start, name_len);
        name[name_len] = '\0';

        i = braced ? end + 1 : end;

        const char *value = get_tab_variable(name, tab_index);

        /* Fall back to the process environment for HOME, PATH, etc. */
        if (value == NULL)
            value = getenv(name);

        if (value != NULL)
        {
            while (*value != '\0' && j + 1 < size)
                dest[j++] = *value++;
        }
    }

    dest[j] = '\0';
}

void clear_tab_variables(int tab_index)
{
    if (tab_index < 0 || tab_index >= MAX_TABS)
        return;

    memset(variables[tab_index], 0, sizeof(variables[tab_index]));
}
