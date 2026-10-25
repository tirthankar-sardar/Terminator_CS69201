
#include "script.h"
#include "gui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <limits.h>

#define MAX_SCRIPT_VARS 100
#define MAX_VAR_NAME 64
#define MAX_VAR_VALUE 1024

static char var_names[MAX_SCRIPT_VARS][MAX_VAR_NAME];
static char var_values[MAX_SCRIPT_VARS][MAX_VAR_VALUE];
static int var_count = 0;

static void script_print(const char *text)
{
    if (text != NULL)
        append_output(text, (int)strlen(text));
}

static char *trim(char *text)
{
    while (isspace((unsigned char)*text))
        text++;

    size_t len = strlen(text);

    while (len > 0 && isspace((unsigned char)text[len - 1]))
        text[--len] = '\0';

    return text;
}

static void make_path(char *result, size_t size,
                      const char *cwd, const char *path)
{
    if (path[0] == '/')
        snprintf(result, size, "%s", path);
    else
        snprintf(result, size, "%s/%s", cwd, path);
}

/* Read a simple test target from: if [ -f "$1" ]; then */

// static int evaluate_file_test(char *line, int argc, char *argv[],
//                               const char *cwd)
// {
//     char *test = strstr(line, "-f");
//     char operation[3] = "-f";

//     if (test == NULL)
//     {
//         test = strstr(line, "-e");
//         strcpy(operation, "-e");
//     }

//     if (test == NULL)
//     {
//         test = strstr(line, "-d");
//         strcpy(operation, "-d");
//     }

//     if (test == NULL)
//         return -1;

//     test += 2;

//     while (isspace((unsigned char)*test))
//         test++;

//     char target[PATH_MAX];
//     size_t i = 0;

//     if (*test == '"' || *test == '\'')
//     {
//         char quote = *test++;

//         while (*test && *test != quote &&
//                i < sizeof(target) - 1)
//             target[i++] = *test++;
//     }
//     else
//     {
//         while (*test && !isspace((unsigned char)*test) &&
//                *test != ']' && i < sizeof(target) - 1)
//             target[i++] = *test++;
//     }

//     target[i] = '\0';

//     if (strcmp(target, "$1") == 0)
//     {
//         if (argc < 2)
//             return 0;

//         snprintf(target, sizeof(target), "%s", argv[1]);
//     }

//     if (target[0] == '\0')
//         return -1;

//     char resolved[PATH_MAX];
//     make_path(resolved, sizeof(resolved), cwd, target);

//     struct stat st;

//     if (stat(resolved, &st) != 0)
//         return 0;

//     if (strcmp(operation, "-f") == 0)
//         return S_ISREG(st.st_mode);

//     if (strcmp(operation, "-d") == 0)
//         return S_ISDIR(st.st_mode);

//     return 1;
// }

static int evaluate_condition(char *line, int argc, char *argv[],
                              const char *cwd)
{

    char *open = strchr(line, '[');
    char *close = open ? strchr(open + 1, ']') : NULL;

    if (!open || !close)
        return -1;

    *close = '\0';
    char *expr = trim(open + 1);

    /* File tests: -f, -e, -d */
    char *p = expr;

    if (strncmp(p, "-f", 2) == 0 ||
        strncmp(p, "-e", 2) == 0 ||
        strncmp(p, "-d", 2) == 0)
    {
        char operation[3];
        operation[0] = p[0];
        operation[1] = p[1];
        operation[2] = '\0';

        p += 2;
        p = trim(p);

        char target[PATH_MAX];
        size_t i = 0;

        if (*p == '"' || *p == '\'')
        {
            char quote = *p++;
            while (*p && *p != quote && i < sizeof(target) - 1)
                target[i++] = *p++;
        }
        else
        {
            while (*p && !isspace((unsigned char)*p) &&
                   i < sizeof(target) - 1)
                target[i++] = *p++;
        }

        target[i] = '\0';

        if (strcmp(target, "$1") == 0)
        {
            if (argc < 2)
                return 0;

            snprintf(target, sizeof(target), "%s", argv[1]);
        }

        if (!target[0])
            return -1;

        char resolved[PATH_MAX];
        make_path(resolved, sizeof(resolved), cwd, target);

        struct stat st;
        if (stat(resolved, &st) != 0)
            return 0;

        if (strcmp(operation, "-f") == 0)
            return S_ISREG(st.st_mode);
        if (strcmp(operation, "-d") == 0)
            return S_ISDIR(st.st_mode);

        return 1;
    }

    /* Numeric tests: -eq and -gt */
    char left[64], operation[8], right[64];

    if (sscanf(expr, "%63s %7s %63s", left, operation, right) != 3)
        return -1;

    char *end;
    long a = strtol(left, &end, 10);
    if (*end != '\0')
        return -1;

    long b = strtol(right, &end, 10);
    if (*end != '\0')
        return -1;

    if (strcmp(operation, "-eq") == 0)
        return a == b;

    if (strcmp(operation, "-gt") == 0)
        return a > b;

    return -1;
}

static const char *get_script_var(const char *name)
{
    for (int i = 0; i < var_count; i++)
    {
        if (strcmp(var_names[i], name) == 0)
            return var_values[i];
    }

    return "";
}

static void set_script_var(const char *name, const char *value)
{
    for (int i = 0; i < var_count; i++)
    {
        if (strcmp(var_names[i], name) == 0)
        {
            snprintf(var_values[i], MAX_VAR_VALUE, "%s", value);
            return;
        }
    }

    if (var_count < MAX_SCRIPT_VARS)
    {
        snprintf(var_names[var_count], MAX_VAR_NAME, "%s", name);
        snprintf(var_values[var_count], MAX_VAR_VALUE, "%s", value);
        var_count++;
    }
}

/* Print a basic quoted or unquoted echo message. */

static int interpret_echo(char *line, int argc, char *argv[])
{
    char *p = trim(line);

    if (strncmp(p, "echo", 4) != 0 ||
        (p[4] != '\0' && !isspace((unsigned char)p[4])))
        return 0;

    p = trim(p + 4);

    /* Remove matching outer quotes. */
    size_t len = strlen(p);

    if (len >= 2 &&
        ((p[0] == '"' && p[len - 1] == '"') ||
         (p[0] == '\'' && p[len - 1] == '\'')))
    {
        p[len - 1] = '\0';
        p++;
    }

    /* Expand positional parameters such as $1 and $2. */
    char output[4096];
    size_t out = 0;

    for (size_t i = 0; p[i] != '\0' && out < sizeof(output) - 1;)
    {
        if (p[i] == '$' &&
            (isdigit((unsigned char)p[i + 1]) ||
             (isalpha((unsigned char)p[i + 1]) || p[i + 1] == '_')))
        {
            i++;

            const char *value = "";
            char name[MAX_VAR_NAME];
            size_t n = 0;

            if (isdigit((unsigned char)p[i]))
            {
                int index = p[i++] - '0';

                if (index == 0 && argc > 0)
                    value = argv[0];
                else if (index > 0 && index < argc)
                    value = argv[index];
            }
            else
            {
                while ((isalnum((unsigned char)p[i]) || p[i] == '_') &&
                       n < sizeof(name) - 1)
                {
                    name[n++] = p[i++];
                }

                name[n] = '\0';
                value = get_script_var(name);
            }

            size_t value_len = strlen(value);

            if (value_len > sizeof(output) - 1 - out)
                value_len = sizeof(output) - 1 - out;

            memcpy(output + out, value, value_len);
            out += value_len;
        }
        else
        {
            output[out++] = p[i++];
        }
    }

    output[out] = '\0';

    script_print(output);
    script_print("\n");
    return 1;
}

static int run_script(const char *script_path, int argc, char *argv[],
                      TerminalTab *tab)
{
    char resolved[PATH_MAX];
    make_path(resolved, sizeof(resolved), tab->cwd, script_path);

    FILE *file = fopen(resolved, "r");

    if (file == NULL)
    {
        char message[PATH_MAX + 64];
        snprintf(message, sizeof(message),
                 "script: cannot open '%s'\n", script_path);
        script_print(message);
        tab->last_status = 1;
        return 1;
    }

    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;

    int inside_if = 0;
    int active = 1;
    int saw_else = 0;
    int branch_taken = 0;
    int failed = 0;

    while ((length = getline(&line, &capacity, file)) != -1)
    {
        (void)length;

        char *command = trim(line);

        if (*command == '\0' || *command == '#')
            continue;

        if (strncmp(command, "if", 2) == 0 &&
            isspace((unsigned char)command[2]))
        {
            if (inside_if)
            {
                script_print("script: nested if is not supported\n");
                failed = 1;
                break;
            }

            inside_if = 1;
            saw_else = 0;
            branch_taken = 0;

            int result = evaluate_condition(command, argc, argv, tab->cwd);

            if (result < 0)
            {
                script_print("script: unsupported condition\n");
                failed = 1;
                break;
            }

            branch_taken = result;
            active = result;
            continue;
        }

        if (strncmp(command, "elif", 4) == 0 &&
            isspace((unsigned char)command[4]))
        {
            if (!inside_if || saw_else)
            {
                script_print("script: unexpected elif\n");
                failed = 1;
                break;
            }

            if (branch_taken)
            {
                active = 0;
                continue;
            }

            int result = evaluate_condition(command, argc, argv, tab->cwd);

            if (result < 0)
            {
                script_print("script: unsupported condition\n");
                failed = 1;
                break;
            }

            branch_taken = result;
            active = result;
            continue;
        }

        if (strcmp(command, "else") == 0)
        {
            if (!inside_if || saw_else)
            {
                script_print("script: unexpected else\n");
                failed = 1;
                break;
            }

            saw_else = 1;
            active = !branch_taken;
            branch_taken = 1;
            continue;
        }

        if (strcmp(command, "fi") == 0)
        {
            if (!inside_if)
            {
                script_print("script: unexpected fi\n");
                failed = 1;
                break;
            }

            inside_if = 0;
            active = 1;
            continue;
        }

        if (!active)
            continue;

        /* Handle simple assignments such as NAME=value. */
        char *equals = strchr(command, '=');

        if (equals != NULL && equals != command)
        {
            int valid_assignment = 1;

            for (char *q = command; q < equals; q++)
            {
                if (!(isalnum((unsigned char)*q) || *q == '_') ||
                    (q == command && isdigit((unsigned char)*q)))
                {
                    valid_assignment = 0;
                    break;
                }
            }

            if (valid_assignment)
            {
                *equals = '\0';

                char *name = trim(command);
                char *value = trim(equals + 1);

                size_t value_len = strlen(value);

                if (value_len >= 2 &&
                    ((value[0] == '"' && value[value_len - 1] == '"') ||
                     (value[0] == '\'' && value[value_len - 1] == '\'')))
                {
                    value[value_len - 1] = '\0';
                    value++;
                }

                set_script_var(name, value);
                continue;
            }
        }

        if (!interpret_echo(command, argc, argv))
        {
            char message[256];
            snprintf(message, sizeof(message),
                     "script: unsupported command: %.180s\n", command);
            script_print(message);
            failed = 1;
            break;
        }
    }

    if (!failed && inside_if)
    {
        script_print("script: missing fi\n");
        failed = 1;
    }

    free(line);
    fclose(file);

    tab->last_status = failed ? 1 : 0;
    return 1;
}

int script_handle_command(int argc, char *argv[], TerminalTab *tab)
{
    if (argc < 1 || argv == NULL || tab == NULL)
        return 0;

    int explicit_script =
        strcmp(argv[0], "source") == 0 ||
        strcmp(argv[0], ".") == 0 ||
        strcmp(argv[0], "bash") == 0;

    size_t len = strlen(argv[0]);
    int is_script = len >= 3 &&
                    strcmp(argv[0] + len - 3, ".sh") == 0;

    if (!explicit_script && !is_script)
        return 0;

    if (explicit_script && argc < 2)
    {
        script_print("script: missing filename\n");
        tab->last_status = 2;
        return 1;
    }

    const char *path = explicit_script ? argv[1] : argv[0];

    char resolved[PATH_MAX];
    make_path(resolved, sizeof(resolved), tab->cwd, path);

    struct stat st;

    if (stat(resolved, &st) != 0 || !S_ISREG(st.st_mode))
    {
        char message[PATH_MAX + 64];
        snprintf(message, sizeof(message),
                 "script: cannot access '%s'\n", path);
        script_print(message);
        tab->last_status = 1;
        return 1;
    }

    return run_script(path, argc - (explicit_script ? 1 : 0),
                      explicit_script ? argv + 1 : argv, tab);
}
