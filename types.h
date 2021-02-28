#ifndef HEADER_TYPES
#define HEADER_TYPES

#include <stdbool.h>
#include <xcb/xcb.h>

// Number of total possible tabs
#define TABS 3

// String literals for shortcut actions' arguments
#define LEFT	"left"
#define RIGHT	"right"
#define UP		"up"
#define DOWN	"down"
#define PARENT	"parent"
#define CHILD	"child"

typedef enum layout_t {HORIZONTAL, VERTICAL} layout_t;
typedef enum keycode_t {\
	Key_Space = 65,
	Key_Return = 36,
	Key_Backspace = 22,
	Key_Up = 111,
	Key_Down = 116,
	Key_Left = 113,
	Key_Right = 114,
	Key_Num_Up = 80,
	Key_Num_Down = 88,
	Key_Num_Left = 83,
	Key_Num_Right = 85,
	Key_Q = 24,
	Key_T = 28,
	Key_E = 26,
	Key_D = 40,
	Key_X = 53,
	Key_V = 55,
	Key_1 = 10,
	Key_2 = 11,
	Key_3 = 12,
	Key_4 = 13,
	Key_5 = 14,
} keycode_t;

typedef enum keymod_t {\
	Mod_Shift = XCB_MOD_MASK_SHIFT,
	Mod_Ctrl = XCB_MOD_MASK_CONTROL,
	Mod_Alt = XCB_MOD_MASK_1,
	Mod_Super = XCB_MOD_MASK_4,
	Mod_CapsLock = XCB_MOD_MASK_LOCK,
    Mod_NumLock = XCB_MOD_MASK_2,
    Mod_Switch = XCB_MOD_MASK_5,
} keymod_t;

typedef struct node_t {
	xcb_window_t	id;
	struct node_t	*parent;
	struct node_t	*prev;
	struct node_t	*next;
	struct node_t	*children;

	int				x;
	int				y;
	unsigned int	w;
	unsigned int	h;

	bool			isactive;
	layout_t		layout;
	pid_t			pid;
} node_t;

typedef struct xinfo_t {
	xcb_connection_t	*conn;
	xcb_screen_t		*screen;
	xcb_window_t		window;
	xcb_window_t		proxy;
	unsigned			width;
	unsigned			height;
} xinfo_t;

typedef struct context_t {
	int				tab;
	node_t			*root[TABS];
	node_t			*active_container[TABS];
	node_t			*active_client[TABS];
	node_t			*stack;
} context_t;

typedef struct shortcut_t {
	keymod_t		mod;
	keycode_t		key;
	void			(*action)(xinfo_t *, context_t *, char *);
	char			*arg;
} shortcut_t;

#endif
