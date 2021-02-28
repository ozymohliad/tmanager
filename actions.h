// Actions bound to shortcuts

#ifndef HEADER_ACTIONS
#define HEADER_ACTIONS

#include "types.h"

void debug(xinfo_t *, context_t *, char *);

// Creates simple X window for testing purposes
void create_test_window(xinfo_t *, context_t *, char *);

// Opens up an external program and reparents its window
void create_client_window(xinfo_t *, context_t *, char *);

// Selects active window
void select_window(xinfo_t *, context_t *, char *);

// Moves active window around
void move_window(xinfo_t *, context_t *, char *);

// Reizes active window
void resize_window(xinfo_t *, context_t *, char *);

// Closes active window and its children
void delete_window(xinfo_t *, context_t *, char *);

// Sets opposite layout for active window
// If window has children, rotates them
// Else, influences where new windows will be placed
void switch_layout(xinfo_t *, context_t *, char *);

// Hides active window and places it on a special stack of hidden windows
void cut_window(xinfo_t *, context_t *, char *);

// Removes last hidden window and inserts it before the active window
void paste_window(xinfo_t *, context_t *, char *);

// Switches to different tab
void switch_tab(xinfo_t *, context_t *, char *);

// Removes active window from current tab and inserts it in another
void move_to_tab(xinfo_t *, context_t *, char *);

#endif
