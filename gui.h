
#ifndef GUI_H
#define GUI_H

int gui_init(void);
void gui_draw(void);
void gui_cleanup(void);
void append_output(const char *text, int length);
void gui_scroll(int lines);

void gui_switch_tab(int index);
void gui_new_tab(void);
void gui_close_tab(void);
void gui_handle_button(XButtonEvent *event);

#endif
