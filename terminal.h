
#ifndef TERMINAL_H
#define TERMINAL_H

#include <X11/Xlib.h>
#include <sys/types.h>

#define INPUT_SIZE 256
#define OUTPUT_SIZE 32768
#define MAX_TABS 8


typedef struct
{
    char input[INPUT_SIZE];
    int input_length;

    char output[OUTPUT_SIZE];
    int output_length;

    int scroll_offset;

    pid_t pid;
    int output_fd;
    int process_running;
    int last_status;
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
