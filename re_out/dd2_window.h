#ifndef DD2_WINDOW_H
#define DD2_WINDOW_H
/* Platform window events enter the registered original WndProc. Physical
 * state synchronization alone is not a keyboard/window message. */
void dd2_window_focus(int active);
void dd2_window_message(void);
unsigned dd2_window_wait_count(int returned);
#endif
