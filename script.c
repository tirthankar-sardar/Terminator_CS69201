#include "script.h"
#include "gui.h"
#include "shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <limits.h>

#define MAX_SCRIPT_VARS 100
#define MAX_VAR_NAME 64
#define MAX_VAR_VALUE 1024

#define MAX_IF_DEPTH 64

static char var_names[MAX_SCRIPT_VARS][MAX_VAR_NAME];
static char var_values[MAX_SCRIPT_VARS][MAX_VAR_VALUE];
static int var_count = 0;

int capture_script_command(const char *command, TerminalTab *tab,
                           char *result, size_t result_size);

typedef struct
{
    int parent_active;
    int active;
    int branch_taken;
    int saw_else;
} IfState;

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

static const char *get_script_var(const char *name);

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
        char operation[3] = {p[0], p[1], '\0'};
        p = trim(p + 2);

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

        return 1; /* -e: any existing file-system entry */
    }

    /* Parse two operands and an operator. */
    char left[256], operation[8], right[256];

    if (sscanf(expr, " %255s %7s %255s",
               left, operation, right) != 3)
        return -1;

    /* Remove matching outer quotes from each operand. */
    char *operands[] = {left, right};

    for (int i = 0; i < 2; i++)
    {
        size_t len = strlen(operands[i]);

        if (len >= 2 &&
            ((operands[i][0] == '"' &&
              operands[i][len - 1] == '"') ||
             (operands[i][0] == '\'' &&
              operands[i][len - 1] == '\'')))
        {
            memmove(operands[i], operands[i] + 1, len - 2);
            operands[i][len - 2] = '\0';
        }
    }

    /* Expand simple variables and positional parameters. */
    char *values[] = {left, right};

    for (int i = 0; i < 2; i++)
    {
        if (values[i][0] != '$')
            continue;

        const char *replacement = "";

        if (isdigit((unsigned char)values[i][1]))
        {
            int index = values[i][1] - '0';

            if (index == 0 && argc > 0)
                replacement = argv[0];
            else if (index > 0 && index < argc)
                replacement = argv[index];
        }
        else if (values[i][1] == '_' ||
                 isalpha((unsigned char)values[i][1]))
        {
            replacement = get_script_var(values[i] + 1);
        }

        snprintf(values[i], 256, "%s", replacement);
    }

    /* String equality. */
    if (strcmp(operation, "=") == 0)
        return strcmp(left, right) == 0;

    /* Numeric comparisons. */
    char *end = NULL;
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

static int interpret_echo(char *line, int argc, char *argv[],
                          int last_status)
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

    /* Expand positional parameters, variables, $# and $?. */
    char output[4096];
    size_t out = 0;

    for (size_t i = 0; p[i] != '\0' && out < sizeof(output) - 1;)
    {
        const char *value = NULL;
        char number[32];
        char name[MAX_VAR_NAME];
        size_t n = 0;

        // if (p[i] == '$' && p[i + 1] == '#')
        // {
        //     snprintf(number, sizeof(number), "%d", argc - 1);
        //     value = number;
        //     i += 2;
        // }
        // else if (p[i] == '$' && p[i + 1] == '?')
        // {
        //     snprintf(number, sizeof(number), "%d", last_status);
        //     value = number;
        //     i += 2;
        // }
        // else if (p[i] == '$' &&
        //          (isdigit((unsigned char)p[i + 1]) ||
        //           isalpha((unsigned char)p[i + 1]) ||
        //           p[i + 1] == '_'))
        // {
        //     i++;

        //     if (isdigit((unsigned char)p[i]))
        //     {
        //         int index = p[i++] - '0';

        //         if (index == 0 && argc > 0)
        //             value = argv[0];
        //         else if (index > 0 && index < argc)
        //             value = argv[index];
        //         else
        //             value = "";
        //     }
        //     else
        //     {
        //         while ((isalnum((unsigned char)p[i]) || p[i] == '_') &&
        //                n < sizeof(name) - 1)
        //         {
        //             name[n++] = p[i++];
        //         }

        //         name[n] = '\0';
        //         value = get_script_var(name);
        //     }
        // }

        if (p[i] == '$' && p[i + 1] == '#')
        {
            snprintf(number, sizeof(number), "%d", argc - 1);
            value = number;
            i += 2;
        }
        else if (p[i] == '$' && p[i + 1] == '?')
        {
            snprintf(number, sizeof(number), "%d", last_status);
            value = number;
            i += 2;
        }
        else if (p[i] == '$' && isdigit((unsigned char)p[i + 1]))
        {
            int index = p[i + 1] - '0';

            if (index < argc && argv[index] != NULL)
                value = argv[index];
            else
                value = "";

            i += 2;
        }
        else if (p[i] == '$' &&
                 (isalpha((unsigned char)p[i + 1]) ||
                  p[i + 1] == '_'))
        {
            i++;

            while ((isalnum((unsigned char)p[i]) || p[i] == '_') &&
                   n < sizeof(name) - 1)
            {
                name[n++] = p[i++];
            }

            name[n] = '\0';
            value = get_script_var(name);

            if (value == NULL)
                value = "";
        }

        else
        {
            output[out++] = p[i++];
            continue;
        }

        size_t value_len = strlen(value);

        if (value_len > sizeof(output) - 1 - out)
            value_len = sizeof(output) - 1 - out;

        memcpy(output + out, value, value_len);
        out += value_len;
    }

    output[out] = '\0';

    script_print(output);
    script_print("\n");
    return 1;
}

static int validate_script_syntax(const char *script_path)
{
    FILE *file = fopen(script_path, "r");

    if (file == NULL)
        return 0;

    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;

    int depth = 0;
    int saw_else[MAX_IF_DEPTH] = {0};
    int valid = 1;
    int line_number = 0;

    while ((length = getline(&line, &capacity, file)) != -1)
    {
        (void)length;
        line_number++;

        char *command = trim(line);

        if (*command == '\0' || *command == '#')
            continue;

        /* Validate if/elif conditions before executing any commands. */
        if ((strncmp(command, "if ", 3) == 0) ||
            (strncmp(command, "elif ", 5) == 0))
        {
            const char *condition =
                (command[0] == 'i') ? command + 3 : command + 5;

            while (isspace((unsigned char)*condition))
                condition++;

            if (*condition == '[')
            {
                const char *close = strchr(condition + 1, ']');

                if (close == NULL)
                {
                    char message[128];

                    snprintf(message, sizeof(message),
                             "script: syntax error near line %d: missing ']'\n",
                             line_number);

                    script_print(message);

                    free(line);
                    fclose(file);
                    return 0;
                }

                /* Reject unexpected text after the closing bracket. */
                close++;
                while (isspace((unsigned char)*close))
                    close++;

                if (*close != '\0' && *close != ';')
                {
                    fprintf(stderr,
                            "script: syntax error near line %d\n",
                            line_number);

                    free(line);
                    fclose(file);
                    return 0;
                }
            }
        }

        if (strncmp(command, "if", 2) == 0 &&
            isspace((unsigned char)command[2]))
        {
            if (depth >= MAX_IF_DEPTH)
            {
                valid = 0;
                break;
            }

            saw_else[depth++] = 0;
        }
        else if (strncmp(command, "elif", 4) == 0 &&
                 isspace((unsigned char)command[4]))
        {
            if (depth == 0 || saw_else[depth - 1])
            {
                valid = 0;
                break;
            }
        }
        else if (strcmp(command, "else") == 0)
        {
            if (depth == 0 || saw_else[depth - 1])
            {
                valid = 0;
                break;
            }

            saw_else[depth - 1] = 1;
        }
        else if (strcmp(command, "fi") == 0)
        {
            if (depth == 0)
            {
                valid = 0;
                break;
            }

            depth--;
        }
    }

    if (depth != 0)
        valid = 0;

    free(line);
    fclose(file);

    if (!valid)
    {
        char message[128];

        snprintf(message, sizeof(message),
                 "script: syntax error near line %d\n", line_number);

        script_print(message);
    }

    return valid;
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

    if (!validate_script_syntax(resolved))
    {
        fclose(file);
        tab->last_status = 2;
        return 1;
    }

    rewind(file);

    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;

    IfState if_stack[MAX_IF_DEPTH];
    int if_depth = 0;

    int active = 1;
    int failed = 0;
    int script_exited = 0;
    int exit_status = 0;

    while ((length = getline(&line, &capacity, file)) != -1)
    {
        (void)length;

        char *command = trim(line);

        if (*command == '\0' || *command == '#')
            continue;

        if (strncmp(command, "if", 2) == 0 &&
            isspace((unsigned char)command[2]))
        {
            if (if_depth >= MAX_IF_DEPTH)
            {
                script_print("script: maximum if nesting depth exceeded\n");
                failed = 1;
                break;
            }

            IfState *state = &if_stack[if_depth++];
            state->parent_active = active;
            state->active = 0;
            state->branch_taken = 0;
            state->saw_else = 0;

            if (active)
            {
                int result = evaluate_condition(command, argc, argv, tab->cwd);

                if (result < 0)
                {
                    script_print("script: unsupported condition\n");
                    failed = 1;
                    break;
                }

                state->active = result;
                state->branch_taken = result;
            }

            active = state->parent_active && state->active;
            continue;
        }

        if (strncmp(command, "elif", 4) == 0 &&
            isspace((unsigned char)command[4]))
        {
            if (if_depth == 0 || if_stack[if_depth - 1].saw_else)
            {
                script_print("script: unexpected elif\n");
                failed = 1;
                break;
            }

            IfState *state = &if_stack[if_depth - 1];

            if (!state->parent_active || state->branch_taken)
            {
                state->active = 0;
            }
            else
            {
                int result = evaluate_condition(command, argc, argv, tab->cwd);

                if (result < 0)
                {
                    script_print("script: unsupported condition\n");
                    failed = 1;
                    break;
                }

                state->active = result;

                if (result)
                    state->branch_taken = 1;
            }

            active = state->parent_active && state->active;
            continue;
        }

        if (strcmp(command, "else") == 0)
        {
            if (if_depth == 0 || if_stack[if_depth - 1].saw_else)
            {
                script_print("script: unexpected else\n");
                failed = 1;
                break;
            }

            IfState *state = &if_stack[if_depth - 1];
            state->saw_else = 1;
            state->active = !state->branch_taken;
            state->branch_taken = 1;

            active = state->parent_active && state->active;
            continue;
        }

        if (strcmp(command, "fi") == 0)
        {
            if (if_depth == 0)
            {
                script_print("script: unexpected fi\n");
                failed = 1;
                break;
            }

            if_depth--;
            active = if_depth == 0
                         ? 1
                         : (if_stack[if_depth].parent_active &&
                            if_stack[if_depth].active);
            continue;
        }

        if (!active)
            continue;

        /* Handle exit [n] without terminating Terminator itself. */
        if (strncmp(command, "exit", 4) == 0 &&
            (command[4] == '\0' || isspace((unsigned char)command[4])))
        {
            char *arg = trim(command + 4);

            if (*arg == '\0')
            {
                exit_status = tab->last_status;
            }
            else
            {
                char *end = NULL;
                long value = strtol(arg, &end, 10);

                while (end != NULL && isspace((unsigned char)*end))
                    end++;

                if (end == arg || (end != NULL && *end != '\0'))
                {
                    script_print("script: exit: numeric argument required\n");
                    exit_status = 2;
                }
                else
                {
                    exit_status = (int)(value & 255);
                }
            }

            script_exited = 1;
            break;
        }

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

                /* Handle simple command substitution: NAME=$(command). */
                if (value[0] == '$' && value[1] == '(')
                {
                    char *closing = strrchr(value, ')');

                    if (closing == NULL)
                    {
                        script_print("script: missing ')' in command substitution\n");
                        failed = 1;
                        break;
                    }

                    *closing = '\0';

                    char captured[4096];
                    int command_status = capture_script_command(
                        value + 2, tab, captured, sizeof(captured));

                    set_script_var(name, captured);
                    tab->last_status = command_status;
                    continue;
                }

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

        /* Handle echo directly only when no redirection or pipe is present. */
        if (strchr(command, '>') == NULL &&
            strchr(command, '<') == NULL &&
            strchr(command, '|') == NULL &&
            interpret_echo(command, argc, argv, tab->last_status))
        {
            continue;
        }

        /* Execute ordinary commands using Terminator's existing shell. */
        // execute_script_line(command, tab);
        /* Execute ordinary script commands synchronously. */
        tab->last_status = execute_script_line_sync(command, tab);
    }

    if (!failed && if_depth > 0)
    {
        script_print("script: missing fi\n");
        failed = 1;
    }

    free(line);
    fclose(file);

    if (script_exited)
        tab->last_status = exit_status;
    else if (failed)
        tab->last_status = 1;

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
