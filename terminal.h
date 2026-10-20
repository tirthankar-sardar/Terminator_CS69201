
#ifndef TERMINAL_H
#define TERMINAL_H

#include <X11/Xlib.h>
#include <sys/types.h>
#include <limits.h>
#include <time.h>

#define INPUT_SIZE 256
#define OUTPUT_SIZE 32768
#define MAX_TABS 8
#define MAX_PIPE_COMMANDS 128

#define MAX_WATCH_COMMANDS 8
#define WATCH_INTERVAL 2

typedef struct
{
    char command[INPUT_SIZE];

    pid_t child_pids[MAX_PIPE_COMMANDS];
    int child_count;

    int output_fd;
    int running;

    time_t next_run;

    int output_started;
    time_t run_timestamp;
} WatchCommand;

typedef struct
{
    char input[INPUT_SIZE];
    int input_length;

    char output[OUTPUT_SIZE];
    int output_length;

    int continuation_mode;
    int scroll_offset;
    char cwd[PATH_MAX];

    // pid_t pid;
    // int output_fd;
    // int process_running;
    // int last_status;

    pid_t pid;
    pid_t child_pids[MAX_PIPE_COMMANDS];
    int child_count;

    int output_fd;
    int process_running;
    int last_status;

    int multiwatch_active;

    int watch_command_count;
    WatchCommand watch_commands[MAX_WATCH_COMMANDS];

} TerminalTab;

extern Display *display;
extern Window window;
extern GC gc;

extern char input[INPUT_SIZE];
extern int input_length;

extern char output[OUTPUT_SIZE];
extern int output_length;

extern int running;

extern TerminalTab tabs[MAX_TABS];
extern int tab_count;
extern int active_tab;

void append_output(const char *text, int length);
int gui_init(void);
void gui_draw(void);
void gui_cleanup(void);
void gui_scroll(int lines);

void handle_keypress(XKeyEvent *event);
void execute_command(void);

#endif
