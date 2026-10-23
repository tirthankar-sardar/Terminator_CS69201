
#ifndef HISTORY_H
#define HISTORY_H

#define HISTORY_MAX_COMMANDS 10000
#define HISTORY_DISPLAY_LIMIT 1000
#define HISTORY_COMMAND_SIZE 256

/* Load saved commands from the history file. */
void history_init(void);

/* Save one command to shared persistent history. */
void history_add(const char *command);

/* Print the most recent commands. */
void history_print(void);

/* Search history using the Ctrl+R search term. */
void history_search(const char *term);

#endif
