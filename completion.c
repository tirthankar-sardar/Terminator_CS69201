
#include "completion.h"
#include "terminal.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <X11/keysym.h>

#define COMPLETION_MAX_MATCHES 100

static char matches[COMPLETION_MAX_MATCHES][NAME_MAX + 1];
static int match_count = 0;
static int pending_start = 0;
static int pending_end = 0;
static int pending = 0;

static int common_prefix_length(const char *a, const char *b)
{
    int i = 0;

    while (a[i] && b[i] && a[i] == b[i])
        i++;

    return i;
}

static void replace_range(
    char *input,
    int *cursor,
    int *length,
    int start,
    int end,
    const char *replacement)
{
    int replacement_length = (int)strlen(replacement);
    int suffix_length = *length - end;

    if (start < 0 || end < start ||
        start > *length || end > *length)
        return;

    if (start + replacement_length + suffix_length >= INPUT_SIZE)
        return;

    memmove(input + start + replacement_length,
            input + end,
            (size_t)(suffix_length + 1));

    memcpy(input + start, replacement, (size_t)replacement_length);

    *length = start + replacement_length + suffix_length;
    *cursor = start + replacement_length;
}

static int has_prefix(const char *name, const char *prefix)
{
    return strncmp(name, prefix, strlen(prefix)) == 0;
}

void completion_handle_tab(
    char *input,
    int *cursor,
    int *length,
    const char *cwd)
{
    if (!input || !cursor || !length || !cwd)
        return;

    /* A new Tab press cancels any previous selection. */
    pending = 0;
    match_count = 0;

    /* Find the start of the word before the cursor. */
    int start = *cursor;

    while (start > 0 &&
           !isspace((unsigned char)input[start - 1]))
        start--;

    char token[INPUT_SIZE];
    int token_length = *cursor - start;

    if (token_length <= 0 || token_length >= INPUT_SIZE)
        return;

    memcpy(token, input + start, (size_t)token_length);
    token[token_length] = '\0';

    /*
     * Split the token into a directory part and filename prefix.
     * Examples:
     *   "de"          -> directory ".", prefix "de"
     *   "src/te"      -> directory "src", prefix "te"
     *   "/tmp/te"     -> directory "/tmp", prefix "te"
     */
    char directory_part[INPUT_SIZE];
    char filename_prefix[INPUT_SIZE];

    char *slash = strrchr(token, '/');

    if (slash)
    {
        size_t dir_length = (size_t)(slash - token);

        if (dir_length == 0)
            snprintf(directory_part, sizeof(directory_part), "/");
        else
        {
            memcpy(directory_part, token, dir_length);
            directory_part[dir_length] = '\0';
        }

        snprintf(filename_prefix, sizeof(filename_prefix),
                 "%s", slash + 1);
    }
    else
    {
        snprintf(directory_part, sizeof(directory_part), ".");
        snprintf(filename_prefix, sizeof(filename_prefix),
                 "%s", token);
    }

    char search_directory[PATH_MAX];

    if (directory_part[0] == '/')
    {
        snprintf(search_directory, sizeof(search_directory),
                 "%s", directory_part);
    }
    else
    {
        int n = snprintf(search_directory, sizeof(search_directory),
                         "%s/%s", cwd, directory_part);

        if (n < 0 || (size_t)n >= sizeof(search_directory))
            return;
    }

    DIR *dir = opendir(search_directory);

    if (!dir)
        return;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if (!has_prefix(entry->d_name, filename_prefix))
            continue;

        if (match_count >= COMPLETION_MAX_MATCHES)
            break;

        snprintf(matches[match_count],
                 sizeof(matches[match_count]),
                 "%s", entry->d_name);

        match_count++;
    }

    closedir(dir);

    if (match_count == 0)
        return;

    pending_start = start;
    pending_end = *cursor;

    /* A single match completes the filename immediately. */
    if (match_count == 1)
    {
        char replacement[INPUT_SIZE];

        if (slash)
            snprintf(replacement, sizeof(replacement),
                     "%.*s%s",
                     (int)(slash - token + 1),
                     token,
                     matches[0]);
        else
            snprintf(replacement, sizeof(replacement),
                     "%s", matches[0]);

        replace_range(input, cursor, length,
                      pending_start, pending_end, replacement);
        return;
    }

    /*
     * Multiple matches: extend to the longest common prefix.
     * Include the directory portion when replacing the token.
     */
    int prefix_length = (int)strlen(matches[0]);

    for (int i = 1; i < match_count; i++)
    {
        int n = common_prefix_length(matches[0], matches[i]);

        if (n < prefix_length)
            prefix_length = n;
    }

    if (prefix_length > (int)strlen(filename_prefix))
    {
        char replacement[INPUT_SIZE];

        if (slash)
            snprintf(replacement, sizeof(replacement),
                     "%.*s%.*s",
                     (int)(slash - token + 1), token,
                     prefix_length, matches[0]);
        else
            snprintf(replacement, sizeof(replacement),
                     "%.*s", prefix_length, matches[0]);

        replace_range(input, cursor, length,
                      pending_start, pending_end, replacement);
    }

    /*
     * If ambiguity remains, display numbered choices.
     * A subsequent number selects one of these entries.
     */
    pending = 1;

    char listing[OUTPUT_SIZE];
    size_t used = 0;

    used += (size_t)snprintf(listing + used,
                             sizeof(listing) - used,
                             "\n");

    for (int i = 0; i < match_count; i++)
    {
        int n = snprintf(listing + used,
                         sizeof(listing) - used,
                         "%d. %s%s",
                         i + 1,
                         matches[i],
                         (i + 1 == match_count) ? "\n" : "   ");

        if (n < 0 || (size_t)n >= sizeof(listing) - used)
            break;

        used += (size_t)n;
    }

    append_output(listing, (int)used);
}

int completion_handle_key(
    KeySym key,
    char *input,
    int *cursor,
    int *length)
{
    if (!pending)
        return 0;

    /* Escape dismisses the numbered choices. */
    if (key == XK_Escape)
    {
        pending = 0;
        match_count = 0;
        return 1;
    }

    /* Only number keys select a completion. */
    if (key < XK_1 || key > XK_9)
        return 0;

    int choice = (int)(key - XK_1);

    if (choice >= match_count)
        return 1;

    char replacement[INPUT_SIZE];

    /*
     * Preserve any directory prefix already typed before the filename.
     * pending_start points to the beginning of the original token.
     */
    int prefix_length = pending_end - pending_start;
    (void)prefix_length;

    /*
     * The input token may already have been extended to its common
     * prefix. Preserve its directory component when selecting.
     */
    int slash_position = -1;

    for (int i = pending_start; i < *cursor; i++)
    {
        if (input[i] == '/')
            slash_position = i;
    }

    int directory_length =
        (slash_position >= pending_start)
            ? slash_position - pending_start + 1
            : 0;

    if (directory_length > 0)
    {
        snprintf(replacement, sizeof(replacement),
                 "%.*s%s",
                 directory_length,
                 input + pending_start,
                 matches[choice]);
    }
    else
    {
        snprintf(replacement, sizeof(replacement),
                 "%s", matches[choice]);
    }

    replace_range(input, cursor, length,
                  pending_start, *cursor, replacement);

    pending = 0;
    match_count = 0;

    return 1;
}
