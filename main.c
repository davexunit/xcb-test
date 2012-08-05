#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_ewmh.h>

typedef struct Window Window;

typedef struct {
     xcb_connection_t *connection;
     xcb_ewmh_connection_t *ewmh;
     Window *window_list;
     Window *active;
} WMState;

struct Window {
     xcb_window_t window_id;
     Window *next;
};

static void
set_window_name(WMState *wm, xcb_window_t window, const char* name) {
     xcb_change_property(wm->connection, XCB_PROP_MODE_REPLACE, window,
			 XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(name), name);
}

static void
set_window_position(WMState *wm, xcb_window_t window, uint32_t x, uint32_t y) {
     const uint32_t position[] = { x, y };
     xcb_configure_window(wm->connection, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, position);
}

static void
set_window_size(WMState *wm, xcb_window_t window, uint32_t width, uint32_t height) {
     const uint32_t size[] = { width, height };
     xcb_configure_window(wm->connection, window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, size);
}

static void
set_window_rect(WMState *wm, xcb_window_t window, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
     const uint32_t rect[] = { x, y, width, height };
     xcb_configure_window(wm->connection, window,
			  XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
			  rect);
}

static void
on_mouse_motion(WMState *wm, xcb_motion_notify_event_t *event) {
}

static void
on_button_press(WMState *wm, xcb_button_press_event_t *event) {
     if(event->state & XCB_MOD_MASK_1) {
	  printf("HOLY SHIT\n");
	  set_window_position(wm, event->event, event->root_x, event->root_y);
     }
}

static void
on_map_request(WMState *wm, xcb_map_request_event_t *event) {
     printf("Map Request:\n"
	    "sequence: %d\n"
	    "parent: %d\n"
	    "window: %d\n",
	    event->sequence,
	    event->parent,
	    event->window);

     xcb_map_window(wm->connection, event->window);
     xcb_flush(wm->connection);
}

static void
on_configure_request(WMState *wm, xcb_configure_request_event_t *event) {
     printf("Configure Request:\n"
	    "stack_mode: %d\n"
	    "sequence: %d\n"
	    "parent: %d\n"
	    "window: %d\n"
	    "sibling: %d\n"
	    "x: %d\n"
	    "y: %d\n"
	    "width: %d\n"
	    "height: %d\n"
	    "border_width: %d\n"
	    "value_mask: %d\n",
	    event->stack_mode,
	    event->sequence,
	    event->parent,
	    event->window,
	    event->sibling,
	    event->x,
	    event->y,
	    event->width,
	    event->height,
	    event->border_width,
	    event->value_mask);

     set_window_rect(wm, event->window, event->x, event->y, event->width, event->height);
}

int
main(int argc, char *argv[]) {
     printf("Starting window manager...\n");

     /* Open connection to X server */
     WMState wm;
     wm.connection = xcb_connect(NULL, NULL);
     
     xcb_ewmh_connection_t ewmh;
     xcb_intern_atom_cookie_t *ewmh_cookies = xcb_ewmh_init_atoms(wm.connection, &ewmh);
     uint8_t status = xcb_ewmh_init_atoms_replies(&ewmh, ewmh_cookies, NULL);
     wm.ewmh = &ewmh;

     /* Get the first screen */
     const xcb_setup_t *setup = xcb_get_setup(wm.connection);
     xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
     xcb_screen_t *screen = iter.data;
     /* uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK; */
     /* uint32_t values[2] = { screen->white_pixel, */
     /* 			    XCB_EVENT_MASK_EXPOSURE }; */
     /* 			   /\* | XCB_EVENT_MASK_BUTTON_PRESS   | */
     /* 			   XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | */
     /* 			   XCB_EVENT_MASK_ENTER_WINDOW   | XCB_EVENT_MASK_LEAVE_WINDOW   | */
     /* 			   XCB_EVENT_MASK_KEY_PRESS      | XCB_EVENT_MASK_KEY_RELEASE    | */
     /* 			   XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY}; */
     /* 			   *\/ */

     /* Print screen information */
     printf("Screen information: %ld\n"
	    "Size: %d x %d\n"
	    "Root depth: %d\n",
	    (long int) screen->root,
	    screen->width_in_pixels,
	    screen->height_in_pixels,
	    screen->root_depth);

     /* Print root window information */
     xcb_window_t root = screen->root;
     printf("Root window information:\n");
	    
     /* Grab server */
     /* xcb_grab_server(connection); */

     const uint32_t select_input_val = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;

     /* Get the current selection owner for the root window. This is the current window manager. */
     /* I guess this is the atom name for the window manager for this screen. */
     char atom_name[6];
     sprintf(atom_name, "WM_S%d", screen->root);
     xcb_intern_atom_cookie_t selection_cookie = xcb_intern_atom(wm.connection, 0, strlen(atom_name), atom_name);
     xcb_intern_atom_reply_t *selection_reply = xcb_intern_atom_reply(wm.connection, selection_cookie, NULL);
     xcb_atom_t selection = selection_reply->atom;
     xcb_get_selection_owner_cookie_t cookie = xcb_get_selection_owner(wm.connection, selection);
     xcb_get_selection_owner_reply_t *selection_owner = xcb_get_selection_owner_reply(wm.connection, cookie, NULL);
     if(selection_owner && selection_owner->owner)
	  printf("Current window manager: %ld\n", (long int) selection_owner->owner);
     
     /* Stop current window manager */
     //xcb_set_selection_owner(connection, , selection, XCB_CURRENT_TIME);

     /* Free shit */
     free(selection_reply);
     free(selection_owner);

     /* This causes an error if some other window manager is running */
     xcb_change_window_attributes(wm.connection,
				  screen->root,
				  XCB_CW_EVENT_MASK, &select_input_val);

     /* Check for error that signifies existence of another window manager */
     xcb_aux_sync(wm.connection);
     
     xcb_generic_event_t *wm_event;
     if((wm_event = xcb_poll_for_event(wm.connection)) != NULL) {
	  printf("Event code: %d\n", wm_event->response_type & ~0x80);
	  printf("Another window manager is already running!\n");
	  exit(EXIT_FAILURE);
     }

     /* Select for events */
     const uint32_t change_win_vals[] = {
	       XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
	       XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
	       XCB_EVENT_MASK_ENTER_WINDOW | 
	       XCB_EVENT_MASK_LEAVE_WINDOW |
	       XCB_EVENT_MASK_STRUCTURE_NOTIFY |
	       XCB_EVENT_MASK_PROPERTY_CHANGE |
	       XCB_EVENT_MASK_BUTTON_PRESS |
	       XCB_EVENT_MASK_BUTTON_RELEASE |
	       XCB_EVENT_MASK_POINTER_MOTION |
	       XCB_EVENT_MASK_FOCUS_CHANGE |
	       XCB_EVENT_MASK_KEY_PRESS
     };

     xcb_change_window_attributes(wm.connection,
				  screen->root,
				  XCB_CW_EVENT_MASK,
				  change_win_vals);

     xcb_query_tree_cookie_t query_tree_cookie = xcb_query_tree(wm.connection, screen->root);
     xcb_query_tree_reply_t *query_tree = xcb_query_tree_reply(wm.connection, query_tree_cookie, NULL);
     
     printf("Query Tree:\n"
	    "length: %d\n"
	    "root: %d\n"
	    "parent: %d\n"
	    "children_len: %d\n",
	    query_tree->length,
	    query_tree->root,
	    query_tree->parent,
	    query_tree->children_len);

     wm.window_list = NULL;
     Window *front = NULL;
     int c;
     for(c = 0; c < xcb_query_tree_children_length(query_tree); ++c) {
	  xcb_window_t window = xcb_query_tree_children(query_tree)[c];
	  printf("Child Window: %d\n", window);
	  xcb_change_window_attributes(wm.connection,
				       window,
				       XCB_CW_EVENT_MASK,
				       change_win_vals);
	  Window *new_window  = (Window *) malloc(sizeof(Window));
	  new_window->window_id = window;
	  new_window->next = NULL;
	  if(front) {
	       front->next = new_window;
	       front = new_window;
	  } else {
	       wm.window_list = new_window;
	       front = new_window;	       
	  }
     }

     Window *current = wm.window_list;
     while(current) {
	  printf("Window: %d\n", current->window_id);
	  current = current->next;
     }

     free(query_tree);

     xcb_flush(wm.connection);

     /* xcb_ungrab_server(connection); */

     /* Create a window */
     /* xcb_window_t window = xcb_generate_id(connection); */
     /* xcb_create_window(connection, */
     /* 		       XCB_COPY_FROM_PARENT, */
     /* 		       window, */
     /* 		       screen->root, */
     /* 		       0, 0, */
     /* 		       150, 150, */
     /* 		       10, */
     /* 		       XCB_WINDOW_CLASS_INPUT_OUTPUT, */
     /* 		       screen->root_visual, */
     /* 		       mask, values); */

     /* Adjust window properties */
     /* set_window_name(connection, window, "Hello, world!"); */
     /* set_window_position(connection, window, screen->width_in_pixels / 2, screen->height_in_pixels / 2); */
     /* set_window_size(connection, window, 640, 480); */
     /* set_window_rect(connection, window, 300, 200, 1024, 600); */

     /* Map thfe window on the screen */
     /* xcb_map_window(connection, window); */

     /* Make sure commands are sent before we pause so that the window gets shown */
     /* xcb_flush(connection); */
     
     printf("Time to listen for events...");
     xcb_generic_event_t *event;
     while((event = xcb_wait_for_event(wm.connection))) {
	  /* printf("Event received: %d\n", event->response_type); */
	  switch(event->response_type & ~0x80) {
	  case XCB_CONFIGURE_REQUEST:
	       on_configure_request(&wm, (xcb_configure_request_event_t *) event);
	       break;
	  case XCB_MAP_REQUEST:
	       on_map_request(&wm, (xcb_map_request_event_t *) event);
	       break;
	  case XCB_EXPOSE:
	       printf("Exposure event for window %ld\n", (long int) ((xcb_expose_event_t *)event)->window);
	       break;
	  case XCB_MOTION_NOTIFY:
	       on_mouse_motion(&wm, (xcb_motion_notify_event_t *) event);
	       break;
	  case XCB_BUTTON_PRESS:
	       on_button_press(&wm, (xcb_button_press_event_t *) event);
	       break;
	  }
	  free(event);
     }

     xcb_disconnect(wm.connection);
     
     return EXIT_SUCCESS;
}
