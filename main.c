#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <xcb/xcb.h>
#include <time.h>

#include "types.h"
#include "config.h"
#include "funcs.h"

// TODO explicitly initialize values to 0/NULL

// Configures given root node and opens a window for it
int init_root_node(xinfo_t *xinfo, node_t *node)
{
	node->id = xcb_generate_id(xinfo->conn);
	node->w = xinfo->width;
	node->h = xinfo->height;
	node->layout = HORIZONTAL;
	
	unsigned mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK;
	unsigned event_mask = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
	unsigned values[] = {0xffffff, BORDER_COLOR, event_mask};
	xcb_create_window(xinfo->conn, XCB_COPY_FROM_PARENT, node->id, xinfo->window, \
			0, 0, node->w, node->h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, \
			xinfo->screen->root_visual, mask, values);
	// Must flush X buffer after calling this function
}

// Initial configuration
int start(xinfo_t *xinfo, context_t *context)
{
	// TODO delete this line - it's for testing
	srand(time(0)); 

	// Establish an X connection and get screen data
	xinfo->conn = xcb_connect(NULL, NULL);
	xinfo->screen = xcb_setup_roots_iterator(xcb_get_setup(xinfo->conn)).data;

	// Create master window
	unsigned mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	unsigned event_mask = XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;
	unsigned values[] = {0xffffff, event_mask};
	xinfo->window = xcb_generate_id(xinfo->conn);
	xinfo->width = DEFAULT_WIDTH;
	xinfo->height = DEFAULT_HEIGHT;
	xcb_create_window(xinfo->conn, XCB_COPY_FROM_PARENT, xinfo->window, xinfo->screen->root, \
			DEFAULT_X, DEFAULT_Y, xinfo->width, xinfo->height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, \
			xinfo->screen->root_visual, mask, values);
	
	// Change window title
	xcb_change_property(xinfo->conn, XCB_PROP_MODE_REPLACE, xinfo->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(TITLE), TITLE);
	xcb_map_window(xinfo->conn, xinfo->window);

	// Create root nodes for every tab
	for (int i = 0; i < TABS; i++)
	{
		context->root[i] = calloc(1, sizeof(node_t));
		context->active_client[i] = NULL;
		context->active_container[i] = context->root[i];
		init_root_node(xinfo, context->root[i]);
	}
	xcb_map_window(xinfo->conn, context->root[0]->id);
	context->tab = 0;
	context->stack = NULL;

	// Create input proxy
	mask = XCB_CW_EVENT_MASK;
	event_mask = XCB_EVENT_MASK_KEY_PRESS;
	values[0] = event_mask;
	xinfo->proxy = xcb_generate_id(xinfo->conn);
	xcb_create_window(xinfo->conn, XCB_COPY_FROM_PARENT, xinfo->proxy, xinfo->window, -1, -1, 1, 1, 0, \
			XCB_WINDOW_CLASS_INPUT_OUTPUT, xinfo->screen->root_visual, mask, values);
	xcb_map_window(xinfo->conn, xinfo->proxy);

	xcb_flush(xinfo->conn);
}

// Run an action bound to some shortcut
int run_action(xinfo_t *xinfo, context_t *context, xcb_mod_mask_t mod, xcb_keycode_t key)
{
    int i;
    int len = sizeof shortcuts / sizeof *shortcuts;
    shortcut_t *s = shortcuts;
    for (i = 0; i < len; s = &shortcuts[++i])
	{
		// Found the shortcut
		if (s->mod == mod && s->key == key) break;
	}
	// Didn't find shortcut
    if (i == len) return 1;

	// Run correspoding action and return its exit code
    (*s->action)(xinfo, context, s->arg);
	
	return 0;
}

// Event loop
int run(xinfo_t *xinfo, context_t *context)
{
	xcb_generic_event_t	*ge;
	while (ge = xcb_wait_for_event(xinfo->conn))
	{
		switch (ge->response_type)
		{
			// Update root node's dimensions when master window gets rescaled
			case XCB_CONFIGURE_NOTIFY:
			{
				xcb_configure_notify_event_t *cne = (xcb_configure_notify_event_t *) ge;
				if (cne->window == xinfo->window)
				{
					int tab = context->tab;
					node_t *root = context->root[tab];
					xinfo->width = root->w = cne->width;
					xinfo->height = root->h = cne->height;
					update_window(xinfo, root);
					rescale_children(xinfo, root, NULL);
					xcb_flush(xinfo->conn);
				}
				break;
			}
			case XCB_FOCUS_IN:
			{
				xcb_set_input_focus(xinfo->conn, XCB_INPUT_FOCUS_NONE, xinfo->proxy, XCB_CURRENT_TIME);
				xcb_flush(xinfo->conn);
				break;
			}
			case XCB_KEY_PRESS:
			{
				xcb_key_press_event_t *e = (xcb_key_press_event_t *) ge;

				// Allow for using shortcuts with various locks on
				xcb_mod_mask_t locks = Mod_CapsLock | Mod_NumLock | Mod_Switch;
				xcb_mod_mask_t mod = e->state & ~locks;

				if (run_action(xinfo, context, mod, e->detail) == 1 && context->active_client[context->tab])
				{
					xcb_window_t w = context->active_client[context->tab]->id;
					e->event = w;
					xcb_send_event(xinfo->conn, false, w, XCB_EVENT_MASK_KEY_PRESS, (char *) e);
					xcb_flush(xinfo->conn);
				}
				break;
			}
		}

		free(ge);
	}
}

void cleanup(xinfo_t *xinfo, context_t *context)
{
	for (int i = 0; i < TABS; i++)
	{
		delete_node(xinfo, context->root[i]);
	}
}

int main()
{
	xinfo_t xinfo;
	context_t context;

	start(&xinfo, &context);
	run(&xinfo, &context);
	cleanup(&xinfo, &context);
}
