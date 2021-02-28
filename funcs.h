#ifndef HEADER_FUNCS
#define HEADER_FUNCS

#include <xcb/xcb.h>
#include "types.h"

int occurences(const char *, int);
char **split_string(const char *);
int random_color();
unsigned int *select_len(node_t *, layout_t);
int *select_coord(node_t *, layout_t);
int count_children(node_t *);
int len_of_children(node_t *);
bool isroot(node_t *);
bool iscontainer(node_t *);
bool isfirst(node_t *);
bool islast(node_t *);
bool isonlychild(node_t *);
void reverse_children(node_t *);
int dist(int, int, int, int);
void get_coordinates(node_t *, int *, int *);
node_t *get_next_container(node_t *, layout_t, int);
node_t *get_innermost_client(node_t *, int, int);
node_t *get_adjacent_node(node_t *, bool);
node_t *get_closest(node_t *);
node_t *get_right(node_t *);
node_t *get_left(node_t *);
node_t *get_top(node_t *);
node_t *get_bottom(node_t *);
node_t *get_parent(node_t *);
node_t *get_child(node_t *);
node_t *get_last(node_t *);
node_t *get_by_direction(node_t *, char *);
void swap_windows(node_t *, node_t *);
void insert_node(node_t *, node_t *, node_t *);
void rescale_children(xinfo_t *, node_t *, node_t *);
void copy_position(node_t *, node_t *);
void copy_size(node_t *, node_t *);
void push_node(xinfo_t *, node_t *);
void push_root(context_t *, xinfo_t *, node_t *);
void pull_node(xinfo_t *, node_t *);
void pull_nodes(xinfo_t *, node_t *);
void init_node(node_t *, node_t *, node_t *);
void open_window(xinfo_t *, node_t *);
void update_window(xinfo_t *, node_t *);
void update_root(xinfo_t *, node_t *);
void set_active(context_t *, xinfo_t *, node_t *);
void detach_node(node_t *);
void delete_node(xinfo_t *, node_t *);
void set_opposite_layout(node_t *);
node_t *remove_redundant(xinfo_t *, node_t *);
void open_client_window(xinfo_t *, node_t *, char *);

#define SWAP(TMP, X, Y) TMP = X; X = Y; Y = TMP

#endif
