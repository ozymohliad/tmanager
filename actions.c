#include <stdio.h> // Required only for debuging

#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include "types.h"
#include "funcs.h"
#include "config.h"

static int count_tree(node_t *node)
{
	if (!node)
		return 0;

	return 1 + count_tree(node->next) + count_tree(node->children);
}

void debug(xinfo_t *xinfo, context_t *context, char *arg)
{
	printf("Number of nodes: %d\n", count_tree(context->root[context->tab]->children));
}

void create_test_window(xinfo_t *xinfo, context_t *context, char *arg)
{
	int tab = context->tab;
	node_t *node = calloc(1, sizeof *node);
	node_t *active = context->active_client[tab];
	node_t *container = context->active_container[tab];

	// If node should be pasted in a split layout, make relevant preparations
	if (active && (isroot(active) || (!iscontainer(active) && active->layout != container->layout)))
	{
		if (isroot(active)) push_root(context, xinfo, active);
		else push_node(xinfo, active);
		container = active->parent;
	}

	init_node(node, container, active);
	open_window(xinfo, node);
	rescale_children(xinfo, container, node);
	set_active(context, xinfo, node);

	xcb_flush(xinfo->conn);
}

void create_client_window(xinfo_t *xinfo, context_t *context, char *arg)
{
	int tab = context->tab;
	node_t *node = calloc(1, sizeof *node);
	node_t *active = context->active_client[tab];
	node_t *container = context->active_container[tab];

	if (active && (isroot(active) || (!iscontainer(active) && active->layout != container->layout)))
	{
		if (isroot(active)) push_root(context, xinfo, active);
		else push_node(xinfo, active);
		container = active->parent;
		xcb_flush(xinfo->conn);
	}

	init_node(node, container, active);
	open_client_window(xinfo, node, arg);

	// Set window's border
	int values[] = {BORDER_COLOR};
	xcb_change_window_attributes(xinfo->conn, node->id, XCB_CW_BORDER_PIXEL, values);

	rescale_children(xinfo, container, node);
	set_active(context, xinfo, node);

	xcb_flush(xinfo->conn);
}

void select_window(xinfo_t *xinfo, context_t *context, char *arg)
{
	node_t *active = context->active_client[context->tab];
	if (!active) return;

	node_t *selected = get_by_direction(active, arg);
	if (!selected) return;

	set_active(context, xinfo, selected);
	xcb_flush(xinfo->conn);
}

void move_window(xinfo_t *xinfo, context_t *context, char *arg)
{
	int tab = context->tab;
	node_t *first = context->active_client[tab];

	// Pull current node on a level higher
	if (strcmp(PARENT, arg) == 0)
	{
		node_t *container = first->parent;

		// If container is root, push everything else down instead
		if (isroot(container))
		{
			if (count_children(container) < 3) return;
			push_root(context, xinfo, container);
		}

		pull_node(xinfo, first);
		first->layout = first->parent->layout;
		remove_redundant(xinfo, container);
		rescale_children(xinfo, first->parent, NULL);
		context->active_container[tab] = first->parent;
	}
	// Push current node one level down
	else if (strcmp(CHILD, arg) == 0)
	{
		if (isonlychild(first)) return;

		// If there are no containers around, create one
		node_t *container = get_adjacent_node(first, true);
		if (!iscontainer(container))
		{
			if (count_children(container->parent) < 3) return;
			container->layout = !container->parent->layout;
			push_node(xinfo, container);
			container = container->parent;
		}
		
		detach_node(first);
		init_node(first, container, NULL);
		container = remove_redundant(xinfo, container->parent);
		rescale_children(xinfo, container, first);
		context->active_container[tab] = first->parent;
	}
	// Else just swap current window with the one specified by direction
	else
	{
		node_t *second = get_by_direction(first, arg);
		if (!second) return;

		swap_windows(first, second);
		set_active(context, xinfo, second);

		update_window(xinfo, first);
		update_window(xinfo, second);
		rescale_children(xinfo, first, NULL);
		rescale_children(xinfo, second, NULL);
	}

	xcb_flush(xinfo->conn);
}

void resize_window(xinfo_t *xinfo, context_t *context, char *arg)
{
	int tab = context->tab;
	node_t *node = context->active_client[tab];
	if (!node || isonlychild(node)) return;

	// Select axis of transformation and which node to resize
	layout_t layout = HORIZONTAL;
	if (strcmp(arg, DOWN) == 0 || strcmp(arg, UP) == 0) layout = VERTICAL;
	while (!isroot(node) && layout != node->parent->layout) node = node->parent;
	if (isroot(node)) return;

	// Get window's parent's size and calculate resize step
	int max_len = *select_len(node->parent, layout);
	int step = RESIZE_METHOD ? RESIZE_STEP_PIXEL : RESIZE_STEP_PERCENT*max_len/100.0;

	// Decide whether to add or subtract resize step to achieve intuitive resize
	// For example, RIGHT increases window's width except when the window
	// is on the very right of its container - in that case user would expect the opposite
	int sign = 1;
	if (strcmp(arg, LEFT) == 0 || strcmp(arg, DOWN) == 0) sign = -1;
	if (layout == HORIZONTAL && islast(node)) sign *= -1;
	if (layout == VERTICAL && isfirst(node)) sign *= -1;

	// Current and new size of the window
	int *len = select_len(node, layout);
	int new_len = *len + sign*step;

	// Keep new len in the boundaries of window's container
	int min_len = abs(step);
	max_len -= min_len;
	if (new_len > max_len) new_len = max_len;
	if (new_len < min_len) new_len = min_len;
	
	// Set new len
	*len = new_len;

	rescale_children(xinfo, node->parent, node);
	xcb_flush(xinfo->conn);
}

void delete_window(xinfo_t *xinfo, context_t *context, char *arg)
{
	int tab = context->tab;
	node_t *node = context->active_client[tab];
	if (!node) return;

	node_t *next = get_closest(node);
	node_t *container = node->parent;
	delete_node(xinfo, node);

	// If there are no more client nodes
	if (!next)
	{
		set_active(context, xinfo, NULL);
		xcb_flush(xinfo->conn);
		return;
	}

	container = remove_redundant(xinfo, container);
	set_active(context, xinfo, next);
	rescale_children(xinfo, container, NULL);
	xcb_flush(xinfo->conn);
}

void switch_layout(xinfo_t *xinfo, context_t *context, char *arg)
{
	int tab = context->tab;
	node_t *node = context->active_client[tab];

	// If no client node, switch root window's layout and exit
	if (!node)
	{
		set_opposite_layout(context->active_container[tab]);
		return;
	}

	set_opposite_layout(node);
	// If node has children, they will have changed their size and location
	if (iscontainer(node))
	{
		rescale_children(xinfo, node, NULL);
		xcb_flush(xinfo->conn);
	}
}

void cut_window(xinfo_t *xinfo, context_t *context, char *arg)
{
	int tab = context->tab;
	node_t *node = context->active_client[tab];
	if (!node) return;

	node_t *next = get_closest(node);
	node_t *container = node->parent;
	
	// Remove node and hide its window
	detach_node(node);
	xcb_reparent_window(xinfo->conn, node->id, context->root[tab]->id, 0, 0);
	xcb_unmap_window(xinfo->conn, node->id);

	// Place node on the stack
	node->parent = NULL;
	node->prev = NULL;
	node->next = context->stack;
	if (context->stack) context->stack->prev = node;
	context->stack = node;

	container = remove_redundant(xinfo, container);
	set_active(context, xinfo, next);
	rescale_children(xinfo, container, NULL);
	xcb_flush(xinfo->conn);
}

void paste_window(xinfo_t *xinfo, context_t *context, char *arg)
{
	node_t *node = context->stack;
	if (!node) return;

	// Remove node from the stack
	if (node->next) node->next->prev = NULL;
	context->stack = node->next;

	int tab = context->tab;
	node_t *active = context->active_client[tab];
	node_t *container = active ? active->parent : context->root[tab];

	// If node should be pasted in a split layout, make relevant preparations
	if (active && (isroot(active) || (!iscontainer(active) && active->layout != container->layout)))
	{
		if (isroot(active)) push_root(context, xinfo, active);
		else push_node(xinfo, active);
		container = active->parent;
	}

	init_node(node, container, active);
	update_window(xinfo, node);
	xcb_map_window(xinfo->conn, node->id);
	rescale_children(xinfo, container, node);
	set_active(context, xinfo, node);
	xcb_flush(xinfo->conn);
}

void switch_tab(xinfo_t *xinfo, context_t *context, char *arg)
{
	int tab = atoi(arg);
	if (tab < 1 || tab > TABS) return;

	// Hide current tab's windows
	xcb_unmap_window(xinfo->conn, context->root[context->tab]->id);
	// And show new tab's
	xcb_map_window(xinfo->conn, context->root[--tab]->id);
	
	context->tab = tab;
	update_root(xinfo, context->root[tab]);
	rescale_children(xinfo, context->root[tab], NULL);

	xcb_flush(xinfo->conn);
}

void move_to_tab(xinfo_t *xinfo, context_t *context, char *arg)
{
	int to_tab = atoi(arg);
	if (to_tab < 1 || to_tab > TABS) return;

	to_tab--;
	int from_tab = context->tab;
	node_t *node = context->active_client[from_tab];
	if (!node || to_tab == from_tab) return;

	// Remove node from current tab
	node_t *next = get_closest(node);
	node_t *container = node->parent;
	detach_node(node);
	set_active(context, xinfo, NULL);

	// Prepare for inserting it in another
	node_t *foreign_active = context->active_client[to_tab];
	node_t *foreign_container = context->active_container[to_tab];
	update_root(xinfo, context->root[to_tab]);

	// Insert it and make it active
	init_node(node, foreign_container, foreign_active);
	context->tab = to_tab;
	set_active(context, xinfo, node);
	context->tab = from_tab;
	rescale_children(xinfo, foreign_container, node);

	// If no nodes left in current tab
	if (!next)
	{
		xcb_flush(xinfo->conn);
		return;
	}

	// Remove possible redundant containers left after our node
	container = remove_redundant(xinfo, container);

	set_active(context, xinfo, next);
	rescale_children(xinfo, container, NULL);
	xcb_flush(xinfo->conn);
}
