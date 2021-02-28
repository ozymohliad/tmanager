main: main.c actions.c funcs.c actions.h funcs.h types.h config.h
	gcc main.c actions.c funcs.c `pkg-config --cflags --libs xcb` -o main
