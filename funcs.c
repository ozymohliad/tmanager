#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <xcb/xcb.h>

#include "types.h"
#include "funcs.h"
#include "config.h"

int occurences(const char *str, int c)
{
	int count = 0;
	for (char *p; p = strchr(str, c); count++, str = ++p);
	return count;
}

char **split_string(const char *str)
{
	int i = 0;
	int words = 1 + occurences(str, ' ');
	char **result = malloc((1 + words) * sizeof *result);
	do
	{
		const char *next = strchr(str, ' ');
		next = next ? next : str + strlen(str);
		int len = next - str;
		result[i] = malloc((len + 1) * sizeof(char));
		strncpy(result[i], str, len);
		result[i][len] = '\0';
		str += len + 1;
	} while (++i < words);

	result[i] = NULL;
	return result;
}

int random_color()
{
	unsigned int max = 0xffffff;
	return rand() % max;
}

unsigned int *select_len(node_t *node, layout_t layout)
{
	return layout == HORIZONTAL ? &(node->w) : &(node->h);
}

int *select_coord(node_t *node, layout_t layout)
{
	return layout == HORIZONTAL ? &(node->x) : &(node->y);
}

int count_children(node_t *node)
{
	int count = 0;
	for (node_t *p = node->children; p; p = p->next, count++);
	return count;
}

int len_of_children(node_t *node)
{
	int count = 0;
	layout_t layout = node->layout;
	for (node_t *p = node->children; p; p = p->next)
	{
		count += *select_len(p, layout);
	}
	return count;
}

bool isroot(node_t *node)
{
	return node->parent == NULL;
}

bool iscontainer(node_t *node)
{
	return node->children != NULL;
}

bool isfirst(node_t *node)
{
	return node->prev == NULL;
}

bool islast(node_t *node)
{
	return node->next == NULL;
}

bool isonlychild(node_t *node)
{
	return isfirst(node) && islast(node);
}

void reverse_children(node_t *container)
{
	node_t *cur = get_last(container);

	if (isfirst(cur)) return;

	container->children = cur;
	for (node_t *p; cur; cur = p)
	{
		p = cur->prev;
		cur->prev = cur->next;
		cur->next = p;
	}
}

// Computes square of the distance between two points
int dist(int x1, int y1, int x2, int y2)
{
	int x = x1-x2;
	int y = y1-y2;
	return x*x + y*y;
}

void get_coordinates(node_t *node, int *x, int *y)
{
	if (!node || !node->parent) return;

	*x += node->x;
	*y += node->y;
	get_coordinates(node->parent, x, y);
}

node_t *get_next_container(node_t *node, layout_t layout, int direction)
{
	if (isroot(node)) return NULL;
	node_t *next;
	if (direction == -1) next = node->prev;
	else if (direction == 1) next = node->next;
	else return NULL;

	if (node->parent->layout == layout && next) return next;
	if (!isroot(node->parent)) return get_next_container(node->parent, layout, direction);
	
	return NULL;
}

node_t *get_innermost_client(node_t *container, int x, int y)
{
	if (!container) return NULL;
	if (!iscontainer(container)) return container;

	// x0 and y0 are container's coordinates relative to the master window's origin
	int x0 = 0;
	int y0 = 0;
	get_coordinates(container, &x0, &y0);

	// Find container's child nearest to (x,y)
	node_t *p = container->children;
	int diff = dist(x, y, x0 + p->x, y0 + p->y);
	for (; p->next && diff > dist(x, y, x0 + p->next->x, y0 + p->next->y); p = p->next)
		diff = dist(x, y, x0 + p->next->x, y0 + p->next->y);

	return get_innermost_client(p, x, y);
}

node_t *get_adjacent_node(node_t *node, bool prefer_containers)
{
	if (!node) return NULL;

	if (node->next && prefer_containers && iscontainer(node->next)) return node->next;
	if (node->prev && prefer_containers && iscontainer(node->prev)) return node->prev;
	if (node->next && !prefer_containers && !iscontainer(node->next)) return node->next;
	if (node->prev && !prefer_containers && !iscontainer(node->prev)) return node->prev;
	if (node->next) return node->next;
	if (node->prev) return node->prev;

	return get_adjacent_node(node->parent, prefer_containers);
}

node_t *get_closest(node_t *node)
{
	int x = 0;
	int y = 0;
	get_coordinates(node, &x, &y);
	return get_innermost_client(get_adjacent_node(node, false), x, y);
}
node_t *get_right(node_t *node)
{
	int x = 0;
	int y = 0;
	get_coordinates(node, &x, &y);
	node_t *next = get_next_container(node, HORIZONTAL, 1);
	return get_innermost_client(next, x, y);
}
node_t *get_left(node_t *node)
{
	int x = 0;
	int y = 0;
	get_coordinates(node, &x, &y);
	node_t *next = get_next_container(node, HORIZONTAL, -1);
	return get_innermost_client(next, x, y);
}
node_t *get_top(node_t *node)
{
	int x = 0;
	int y = 0;
	get_coordinates(node, &x, &y);
	node_t *next = get_next_container(node, VERTICAL, -1);
	return get_innermost_client(next, x, y);
}
node_t *get_bottom(node_t *node)
{
	int x = 0;
	int y = 0;
	get_coordinates(node, &x, &y);
	node_t *next = get_next_container(node, VERTICAL, 1);
	return get_innermost_client(next, x, y);
}
node_t *get_parent(node_t *node)
{
	if (!node->parent || isonlychild(node)) return NULL;
	return node->parent;
}
node_t *get_child(node_t *node)
{
	if (!iscontainer(node)) return NULL;
	return node->children;
}
node_t *get_last(node_t *container)
{
	node_t *p;
	for (p = container->children; p && p->next; p = p->next);
	return p;
}
node_t *get_by_direction(node_t *node, char *direction)
{
	if (strcmp(direction, LEFT) == 0) return get_left(node);
	if (strcmp(direction, RIGHT) == 0) return get_right(node);
	if (strcmp(direction, UP) == 0) return get_top(node);
	if (strcmp(direction, DOWN) == 0) return get_bottom(node);
	if (strcmp(direction, PARENT) == 0) return get_parent(node);
	if (strcmp(direction, CHILD) == 0) return get_child(node);

	return NULL;
}

void swap_windows(node_t *first, node_t *second)
{
	xcb_window_t wtmp;
	SWAP(wtmp, first->id, second->id);

	layout_t ltmp;
	SWAP(ltmp, first->layout, second->layout);

	node_t *ntmp;
	SWAP(ntmp, first->children, second->children);
	for (node_t *p = first->children; p; p = p->next) 
		p->parent = first;
	for (node_t *p = second->children; p; p = p->next)
		p->parent = second;
}

// Inserts given node next to previous (which is a child of container)
// and makes it container's child. If previous is NULL, node is inserted as a first child
void insert_node(node_t *node, node_t *container, node_t *previous)
{
	if (!node) return;

	node->parent = container;
	if (!container) return;

	node->prev = previous;
	if (!previous)
	{
		node->next = container->children;
		container->children = node;
	}
	else
	{
		node->next = previous->next;
		previous->next = node;
	}

	if (node->next) node->next->prev = node;
}

void rescale_children(xinfo_t *xinfo, node_t *container, node_t *ignored)
{
	if (!iscontainer(container)) return;

	layout_t layout = container->layout;
	int ignored_len = ignored && ignored->parent == container ? *select_len(ignored, layout) : 0;
	int new_len = *select_len(container, layout) - ignored_len;
	int old_len = len_of_children(container) - ignored_len;
	float scale_factor = 1.0 * new_len / old_len;

	// Change children's size
	int actual_new_len = 0;
	int opposite_len = *select_len(container, !layout);
	for (node_t *p = container->children; p; p = p->next)
	{
		if (p != ignored)
		{
			int *child_len = select_len(p, layout);
			actual_new_len += (*child_len = 0.5 + *child_len * scale_factor);
		}
		*select_len(p, !layout) = opposite_len;
		if (p->children) rescale_children(xinfo, p, ignored);
	}
	
	// Add a few pixels from truncated decimal parts of child_len to ignored/first node
	*select_len(ignored ? ignored : container->children, layout) += new_len - actual_new_len;

	// Change children's position and update its window
	int pos = 0;
	for (node_t *p = container->children; p; p = p->next)
	{
		*select_coord(p, layout) = pos;
		*select_coord(p, !layout) = 0;
		pos += *select_len(p, layout);
		update_window(xinfo, p);
	}

	// Must update windows after calling this function
}

void copy_position(node_t *to, node_t *from)
{
	to->x = from->x;
	to->y = from->y;
}

void copy_size(node_t *to, node_t *from)
{
	to->w = from->w;
	to->h = from->h;
}

void push_node(xinfo_t *xinfo, node_t *node)
{
	// Supplant given node with another - given node's new parent
	detach_node(node);
	node_t *container = calloc(1, sizeof *container);
	copy_position(container, node);
	copy_size(container, node);
	container->layout = node->layout;
	insert_node(container, node->parent, node->prev);
	open_window(xinfo, container);

	node->parent = container;
	node->prev = NULL;
	node->next = NULL;
	node->x = 0;
	node->y = 0;
	container->children = node;
}

void push_root(context_t *context, xinfo_t *xinfo, node_t *root)
{
	push_node(xinfo, root);
	context->root[context->tab] = root->parent;
	root->parent->layout = !root->layout;
}

void pull_node(xinfo_t *xinfo, node_t *node)
{
	node_t *container = node->parent;

	copy_position(node, container);
	detach_node(node);
	insert_node(node, container->parent, container->prev);
	xcb_reparent_window(xinfo->conn, node->id, node->parent->id, node->x, node->y);
}

void pull_nodes(xinfo_t *xinfo, node_t *container)
{
	node_t *p = container->children;
	for (node_t *n; p; p = n)
	{
		n = p->next;
		pull_node(xinfo, p);
	}
	delete_node(xinfo, container);	
}

void init_node(node_t *node, node_t *container, node_t *next)
{
	if (!node || !container) return;

	// Insert node in container
	layout_t layout = container->layout;
	if (!iscontainer(node)) node->layout = layout;
	insert_node(node, container, next ? next->prev : NULL);

	// Compute node's dimensions
	*select_len(node, layout) = *select_len(container, layout) / count_children(container);
	*select_len(node, !layout) = *select_len(container, !layout);

	// Must call rescale_children on container after this function
}

// Open X simple window in an empty preconfigured node
void open_window(xinfo_t *xinfo, node_t *node)
{
	node->id = xcb_generate_id(xinfo->conn);
	xcb_window_t parent = node->parent ? node->parent->id : xinfo->window;
	unsigned int mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK;
	unsigned int event_mask = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
	unsigned int background = node->parent ? random_color() : 0xffffff; // TODO
	int values[] = {background, BORDER_COLOR, event_mask};
	xcb_create_window(xinfo->conn, XCB_COPY_FROM_PARENT, node->id, parent, \
			node->x, node->y, node->w, node->h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, \
			xinfo->screen->root_visual, mask, values);
	xcb_map_window(xinfo->conn, node->id);
}

void update_window(xinfo_t *xinfo, node_t *node)
{
	unsigned mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | \
					XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
	int width = node->w;
	int height = node->h;
	if (node->isactive)
	{
		width -= 2*BORDER;
		height -= 2*BORDER;
	}
	unsigned values[] = {node->x, node->y, width, height};
	if (!isroot(node))
	{
		xcb_reparent_window(xinfo->conn, node->id, node->parent->id, node->x, node->y);
	}
	xcb_configure_window(xinfo->conn, node->id, mask, values);
}

void update_root(xinfo_t *xinfo, node_t *root)
{
	root->w = xinfo->width;
	root->h = xinfo->height;
	update_window(xinfo, root);
}

void set_active(context_t *context, xinfo_t *xinfo, node_t *node)
{
	int tab = context->tab;
	unsigned mask = XCB_CONFIG_WINDOW_WIDTH |\
					XCB_CONFIG_WINDOW_HEIGHT |\
					XCB_CONFIG_WINDOW_BORDER_WIDTH;
	unsigned values[3];

	// Make previous active window unactive
	if (context->active_client[tab])
	{
		node_t *old = context->active_client[tab];
		old->isactive = false;
		values[0] = old->w;
		values[1] = old->h;
		values[2] = 0;
		xcb_configure_window(xinfo->conn, old->id, mask, values);
	}

	context->active_client[tab] = node;
	
	// Add border to new active and change active container
	if (node)
	{
		node->isactive = true;
		context->active_container[tab] = node->parent;
		values[0] = node->w - 2*BORDER;
		values[1] = node->h - 2*BORDER;
		values[2] = BORDER;
		xcb_configure_window(xinfo->conn, node->id, mask, values);
	}
	else
	{
		context->active_container[tab] = context->root[tab];
		fprintf(stderr, NOTE "set_active: active client is set to NULL\n" END);
	}
}

// Removes given node from the linked list of its siblings
void detach_node(node_t *node)
{
	if (isroot(node)) return;

	if (isfirst(node))
	{
		node->parent->children = node->next;
		if (node->next)
			node->next->prev = NULL;
	}
	else
	{
		node->prev->next = node->next;
		if (node->next)
			node->next->prev = node->prev;
	}
}

// Removes node and destroys its window
void delete_node(xinfo_t *xinfo, node_t *node)
{
	detach_node(node);
	for (node_t *p = node->children; p; p = p->next)
		delete_node(xinfo, p);

	if (isroot(node)) return;

	xcb_destroy_window(xinfo->conn, node->id);

	if (node->pid)
	{
		kill(node->pid, SIGTERM);
		waitpid(node->pid, NULL, 0);
	}

	free(node);
}

void set_opposite_layout(node_t *node)
{
	if (!node) return;

	layout_t layout = !node->layout;
	node->layout = layout;
	if (isonlychild(node) && !isroot(node)) node->parent->layout = layout;
	if (iscontainer(node))
	{
		// Reverse children in every second level of hierarchy to
		// achieve effect of rotation. Change layout in condition
		// below to change direction of rotation
		if (node->layout == HORIZONTAL) reverse_children(node);

		// Switch each child's layout
		for (node_t *p = node->children; p; p = p->next)
		{
			int tmp;
			SWAP(tmp, p->x, p->y);
			SWAP(tmp, p->w, p->h);
			set_opposite_layout(p);
		}
	}
}

// Removes redundancy from the node tree below the provided container
// and returns new container since the provided one may get deleted
node_t *remove_redundant(xinfo_t *xinfo, node_t *container)
{
	int children = count_children(container);
	node_t *new_container = container;
	// If container has at least 2 children, it's not redundant
	if (children > 1) return new_container;
	// If container is empty and non-root, it should be deleted
	// if empty and root, there's nothing to do
	if (children == 0)
	{
		if (!isroot(container))
		{
			new_container = container->parent;
			delete_node(xinfo, container);
		}
		return new_container;
	}

	// Remove redundant nested containers
	while (children == 1)
	{
		node_t *child = container->children;
		if (!iscontainer(child)) break;
		// else child is a redundant intermediate container
		
		// Remove child from the tree
		container->children = child->children;
		container->layout = child->layout;
		for (node_t *p = container->children; p; p = p->next)
		{
			p->parent = container;
			update_window(xinfo, p);
		}

		// Remove child from screen and memory
		xcb_destroy_window(xinfo->conn, child->id);
		free(child);
		
		children = count_children(container);
	}

	if (isroot(container)) return container;
	// Merge container with its parent if appropriate
	if (container->layout == container->parent->layout || children == 1)
	{
		new_container = container->parent;
		pull_nodes(xinfo, container);
	}

	return new_container;
}

void open_client_window(xinfo_t *xinfo, node_t *node, char *template)
{
	if (!node || !node->parent) return;

	char *tmp = strstr(template, "%d");
	if (!tmp)
	{
		fprintf(stderr, ERROR "Wrong command: missing XID placeholder\n" END);
		return;
	}
	else if (strstr(++tmp, "%d"))
	{
		fprintf(stderr, ERROR "Wrong command: too many XID placeholders\n" END);
		return;
	}

	char *command = malloc((strlen(template) + 8) * sizeof *command);
	sprintf(command, template, node->parent->id);
	char **args = split_string(command);

	pid_t child = fork();
	if (child == 0)
	{
		execv(args[0], args);	

		fprintf(stderr, ERROR "Error launching client\n" END);
		exit(1);
	}

	xcb_generic_event_t *ge;
	while ((ge = xcb_wait_for_event(xinfo->conn))->response_type != XCB_REPARENT_NOTIFY);
	xcb_reparent_notify_event_t *rne = (xcb_reparent_notify_event_t *) ge;

	node->id = rne->window;
	node->pid = child;
}
