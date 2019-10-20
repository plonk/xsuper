static int move(Display *display, Window window, unsigned release_button,
		int pressed_x, int pressed_y)
{

    Window root_return, child;
    int current_x=pressed_x;
    int current_y=pressed_y;
    int initial_x, initial_y;
    int win_x, win_y;
    int dummy;
    unsigned int mask;
    XWindowAttributes attr;
    static Cursor cursor=XCreateFontCursor(display, XC_hand2);

    XGetWindowAttributes(display, window, &attr);
    win_x=initial_x=attr.x;
    win_y=initial_y=attr.y;

    XGrabPointer(display, window, False,
		 PointerMotionHintMask | ButtonMotionMask | ButtonReleaseMask |
		 OwnerGrabButtonMask | Expose, GrabModeAsync, GrabModeAsync, None,
		 cursor, CurrentTime);

    for (;;) {
	XEvent event;

	XNextEvent(display, &event);
	switch(event.type){
	case ButtonRelease:
	    if (event.xbutton.button==release_button){
		XUngrabPointer(display, CurrentTime);
		XFlush(display);
		/* throw away all pending button events */
		while(XCheckMaskEvent(display, ButtonPressMask|ButtonReleaseMask,
				      &event));

		/* return, if something has changed */
		return (initial_x!=win_x || initial_y!=win_y);
	    }
	    break;
	case MotionNotify:
	    /* throw away all pending motion events */
	    while(XCheckTypedEvent(display, MotionNotify, &event));

	    /* get mouse position */
	    XQueryPointer(display, DefaultRootWindow(display), &root_return,
			  &child, &current_x, &current_y, &dummy, &dummy,
			  &mask);
	    win_x=initial_x+current_x-pressed_x;
	    win_y=initial_y+current_y-pressed_y;
	    XMoveWindow(display, window, win_x, win_y); break;
	case Expose:
	    printf("However, Expose in move()\n"); break;
	}
    }
}
    

int main()
{
    XNextEvent(display, &event);
    if (event.type==ButtonPress){
	move(display, window, event.xbutton.button, event.xbutton.x_root,
	     event.xbutton.y_root);
    }
}
