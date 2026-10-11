
#include "variables.h"
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
}
