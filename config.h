#ifndef HEADER_CONFIG
#define HEADER_CONFIG

#include "types.h"
#include "actions.h"

#define TITLE			"Terminal Manager"
#define DEFAULT_X		0
#define DEFAULT_Y		0
#define DEFAULT_WIDTH	750
#define DEFAULT_HEIGHT	500
#define BORDER			4
#define BORDER_COLOR	0xffb555

#define RESIZE_METHOD		0	// 0 - PERCENT, 1 (or any non-zero int) - PIXEL
#define RESIZE_STEP_PERCENT 10
#define RESIZE_STEP_PIXEL	25

#define ERROR "\e[91m"
#define WARNING "\e[93m"
#define NOTE "\e[94m"
#define END "\e[0m"

#define MODKEY Mod_Alt
static shortcut_t shortcuts[] = {\
//	Modifier key                Regular key     Function to call        Argument
	{MODKEY,					Key_T,			debug,					NULL},
	{MODKEY,					Key_D,			delete_window,			NULL},
	{MODKEY,					Key_X,			cut_window,				NULL},
	{MODKEY,					Key_V,			paste_window,			NULL},
	{MODKEY,					Key_Return,		create_test_window,		NULL},
	{MODKEY|Mod_Shift,			Key_Return,		create_client_window,	"/usr/bin/alacritty --embed %d"},
	{MODKEY,					Key_Backspace,	switch_layout,			NULL},

	{MODKEY,					Key_Left,		select_window,			LEFT},
	{MODKEY,					Key_Right,		select_window,			RIGHT},
	{MODKEY,					Key_Up,			select_window,			UP},
	{MODKEY,					Key_Down,		select_window,			DOWN},
	{MODKEY,					Key_Num_Up,		select_window,			PARENT},
	{MODKEY,					Key_Num_Down,	select_window,			CHILD},

	{MODKEY|Mod_Shift,			Key_Left,		move_window,			LEFT},
	{MODKEY|Mod_Shift,			Key_Right,		move_window,			RIGHT},
	{MODKEY|Mod_Shift,			Key_Up,			move_window,			UP},
	{MODKEY|Mod_Shift,			Key_Down,		move_window,			DOWN},
	{MODKEY|Mod_Shift,			Key_Num_Up,		move_window,			PARENT},
	{MODKEY|Mod_Shift,			Key_Num_Down,	move_window,			CHILD},

	{Mod_Shift|Mod_Ctrl,		Key_Left, 		resize_window,			LEFT},
	{Mod_Shift|Mod_Ctrl,		Key_Right,		resize_window,			RIGHT},
	{Mod_Shift|Mod_Ctrl,		Key_Up,	 		resize_window,			UP},
	{Mod_Shift|Mod_Ctrl,		Key_Down,		resize_window,			DOWN},

	{MODKEY,					Key_1,			switch_tab,				"1"},
	{MODKEY,					Key_2,			switch_tab,				"2"},
	{MODKEY,					Key_3,			switch_tab,				"3"},

	{MODKEY|Mod_Shift,			Key_1,			move_to_tab,			"1"},
	{MODKEY|Mod_Shift,			Key_2,			move_to_tab,			"2"},
	{MODKEY|Mod_Shift,			Key_3,			move_to_tab,			"3"},
};

#endif
