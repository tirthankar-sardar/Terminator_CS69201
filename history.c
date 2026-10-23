
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "terminal.h"
#include "history.h"

/*
 * All tabs share this one history list.
 * The commands are also saved to a file in the user's home directory.
 */
static char commands[HISTORY_MAX_COMMANDS][HISTORY_COMMAND_SIZE];
static int command_count = 0;
static char history_path[PATH_MAX];

/* Construct the path ~/.terminator_history. */
static int build_history_path(void)
{
    const char *home = getenv("HOME");

    if (home == NULL || *home == '\0')
        return -1;

    int n = snprintf(history_path, sizeof(history_path),
                     "%s/.terminator_history", home);

    if (n < 0 || (size_t)n >= sizeof(history_path))
        return -1;

    return 0;
}

/* Remove the oldest command when the list is full. */
static void remove_oldest_command(void)
{
    if (command_count <= 0)
        return;

    memmove(commands, commands + 1,
            sizeof(commands[0]) * (size_t)(command_count - 1));

    command_count--;
}

/* Add a command to the in-memory list. */
static void add_to_memory(const char *command)
{
    if (command == NULL || *command == '\0')
        return;

    if (command_count == HISTORY_MAX_COMMANDS)
        remove_oldest_command();

    snprintf(commands[command_count],
             sizeof(commands[command_count]), "%s", command);

    command_count++;
}

/* Load history from disk when the application starts. */
void history_init(void)
{
    command_count = 0;

    if (build_history_path() != 0)
        return;

    FILE *file = fopen(history_path, "r");

    if (file == NULL)
        return; /* The file may not exist on the first run. */

    char line[HISTORY_COMMAND_SIZE];

    while (fgets(line, sizeof(line), file) != NULL)
    {
        line[strcspn(line, "\r\n")] = '\0';

        if (*line != '\0')
            add_to_memory(line);
    }

    fclose(file);
}

/* Save a command in memory and append it to the history file. */
void history_add(const char *command)
{
    if (command == NULL || *command == '\0')
        return;

    /* Avoid consecutive duplicate commands. */
    if (command_count > 0 &&
        strcmp(commands[command_count - 1], command) == 0)
        return;

    add_to_memory(command);

    if (history_path[0] == '\0' &&
        build_history_path() != 0)
        return;

    FILE *file = fopen(history_path, "a");

    if (file == NULL)
        return;

    fprintf(file, "%s\n", command);
    fclose(file);

    /*
     * Compact the file if it has grown beyond the limit.
     * This keeps only the latest 10,000 saved commands.
     */
    if (command_count == HISTORY_MAX_COMMANDS)
    {
        file = fopen(history_path, "w");

        if (file != NULL)
        {
            for (int i = 0; i < command_count; i++)
                fprintf(file, "%s\n", commands[i]);

            fclose(file);
        }
    }
}

/* Display the latest 1,000 commands in the GUI terminal. */
void history_print(void)
{
    int start = command_count - HISTORY_DISPLAY_LIMIT;

    if (start < 0)
        start = 0;

    for (int i = start; i < command_count; i++)
    {
        char line[HISTORY_COMMAND_SIZE + 32];

        int n = snprintf(line, sizeof(line),
                         "%d  %s\n", i + 1, commands[i]);

        if (n > 0)
            append_output(line, n);
    }
}

/* Find the longest common substring length of two strings. */
static int longest_common_substring(const char *a, const char *b)
{
    int rows = (int)strlen(a);
    int cols = (int)strlen(b);
    int best = 0;

    int dp[HISTORY_COMMAND_SIZE + 1][HISTORY_COMMAND_SIZE + 1] = {{0}};

    for (int i = 1; i <= rows; i++)
    {
        for (int j = 1; j <= cols; j++)
        {
            if (a[i - 1] == b[j - 1])
            {
                dp[i][j] = dp[i - 1][j - 1] + 1;

                if (dp[i][j] > best)
                    best = dp[i][j];
            }
        }
    }

    return best;
}

/*
 * Search newest-to-oldest for a command containing the complete term.
 * If none exists, show all commands tied for the longest common
 * substring, provided that substring is longer than two characters.
 */

/* Search newest-to-oldest for a command containing the complete term. */
void history_search(const char *term)
{

    fprintf(stderr,
            "DEBUG: history_search called with term: %s\n",
            term);

    if (term == NULL || *term == '\0')
    {
        const char *message = "Please enter a search term.\n";
        append_output(message, (int)strlen(message));
        return;
    }

    int best = 0;

    /* First, look for an exact substring match. */
    for (int i = command_count - 1; i >= 0; i--)
    {
        if (strstr(commands[i], term) != NULL)
        {
            append_output(commands[i], (int)strlen(commands[i]));
            append_output("\n", 1);
            return;
        }
    }

    /* If no exact match exists, find the best fuzzy match. */
    for (int i = 0; i < command_count; i++)
    {
        int score = longest_common_substring(term, commands[i]);

        if (score > best)
            best = score;
    }

    /* Only accept fuzzy matches longer than 2 characters. */
    if (best > 2)
    {
        for (int i = 0; i < command_count; i++)
        {
            if (longest_common_substring(term, commands[i]) != best)
                continue;

            /* Skip commands already printed by an earlier match. */
            int already_printed = 0;

            for (int j = 0; j < i; j++)
            {
                if (longest_common_substring(term, commands[j]) == best &&
                    strcmp(commands[i], commands[j]) == 0)
                {
                    already_printed = 1;
                    break;
                }
            }

            if (!already_printed)
            {
                append_output(commands[i], (int)strlen(commands[i]));
                append_output("\n", 1);
            }
        }
    }
    else
    {
        const char *message = "No matching command found.\n";
        append_output(message, (int)strlen(message));
    }
}
