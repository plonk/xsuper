#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/extensions/shape.h>
#include <locale.h>
#include <X11/Xatom.h>
#include <assert.h>
#include <X11/cursorfont.h>

void SetWindowAlwaysOnTop(void);
void DrawText(Drawable d, unsigned long foregronud, Bool bordering);
XFontSet GetFontSet(void);

Display *disp;
Window win;
char buffer[2048];
char *lines[100];

void CheckShapeExtension(void)
{
    // XShapeQueryExtension に NULL は渡せないので一応出力用変数を用意する。
    int event_base, error_base;
    Bool supported = XShapeQueryExtension(disp, &event_base, &error_base);

    if (!supported) {
	printf("non-rectangular window NOT supported\n");
	exit(1);
    }
}

void SetWindowType()
{
    Atom window_type = XInternAtom(disp, "_NET_WM_WINDOW_TYPE", False);
    Atom notification = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_NOTIFICATION", False);

    assert(window_type != None);
    assert(notification != None);

    XChangeProperty(disp, win,
		    window_type,
		    XA_ATOM,
		    32,
		    PropModeReplace,
		    (void *) &notification,
		    1);
}

void CreateWindow(void)
{
    win = XCreateSimpleWindow(disp, DefaultRootWindow(disp), 0, 0, 320, 240, 0, 0, WhitePixel(disp, DefaultScreen(disp)));

    // make window override-redirect
    XSetWindowAttributes attrs;
    attrs.override_redirect = False;
    attrs.backing_store = NotUseful;
    // attrs.backing_store = Always;
    {
	Colormap cm = DefaultColormap(disp, DefaultScreen(disp));
	XColor border;

	XParseColor(disp, cm, "#000080", &border);
	XAllocColor(disp, cm, &border);
	attrs.background_pixel = border.pixel;
    }
    XChangeWindowAttributes(disp, win, CWOverrideRedirect | CWBackingStore | CWBackPixel, &attrs);



    // 
    SetWindowType();

    XMapWindow(disp, win);

    Atom WM_DELETE_WINDOW = XInternAtom(disp, "WM_DELETE_WINDOW", False); 
    XSetWMProtocols(disp, win, &WM_DELETE_WINDOW, 1);

    XSelectInput(disp, win, ExposureMask | ButtonPressMask);

    XClassHint hint = {
	"XSuper",
	"XSuper",
    };

    XSetClassHint(disp, win, &hint);
    XStoreName(disp, win, "XSuper");

    SetWindowAlwaysOnTop();
}

void OpenDisplay(void)
{
    disp = XOpenDisplay(NULL);
}

void SetLocale()
{
    setlocale(LC_ALL, "");
}

void SetText(const char *text)
{
    static char buffer[2048];

    strcpy(buffer, text);

    char *p = buffer;
    int i = 0;

    while (1) {
	lines[i++] = p;
	p = index(p, '\n');
	if (!p)
	    break;
	*p++ = '\0';
    }
    lines[i] = NULL;
}

void Initialize(void)
{
    fread(buffer, 1, 2048, stdin);
    SetText(buffer);
    
    SetLocale();
    OpenDisplay();
    CheckShapeExtension();
    CreateWindow();
}

void DrawText(Drawable d, unsigned long foreground, Bool bordering)
{
    int lmargin = 10, tmargin = 10;
    XFontSet fontset = GetFontSet();
    int linetop = tmargin;
    int line_spacing = 5;
    int border_width = 2;

    GC gc = XCreateGC(disp, d, 0, NULL);
    XSetForeground(disp, gc, foreground);

    int i;
    for (i = 0; lines[i]; i++) {
	XRectangle ext;

	Xutf8TextExtents(fontset, lines[i], strlen(lines[i]), NULL, &ext);

	if (bordering) {
	    int xoff;
	    int yoff;
	    for (yoff = -border_width; yoff <= border_width; yoff++) {
		for (xoff = -border_width; xoff <= border_width; xoff++) {
		    Xutf8DrawString(disp, d, fontset, gc,
				    lmargin - ext.x + xoff,
				    linetop - ext.y + yoff,
				    lines[i],
				    strlen(lines[i]));
		}
	    }
	} else {
	    Xutf8DrawString(disp, d, fontset, gc,
			    lmargin - ext.x,
			    linetop - ext.y,
			    lines[i],
			    strlen(lines[i]));
	}

	linetop += ((ext.height < 24) ? 24 : ext.height)  + line_spacing;
    }

    XFreeGC(disp, gc);
}

#define _NET_WM_STATE_REMOVE        0
#define _NET_WM_STATE_ADD           1
#define _NET_WM_STATE_TOGGLE        2

void SetWindowAlwaysOnTop(void)
{
    Atom wmStateAbove = XInternAtom( disp, "_NET_WM_STATE_ABOVE", 1 );

    if( wmStateAbove == None ) {
	printf( "ERROR: cannot find atom for _NET_WM_STATE_ABOVE !\n" );
	return;
    }

    Atom wmNetWmState = XInternAtom( disp, "_NET_WM_STATE", 1 );
    if( wmNetWmState == None ) {
	printf( "ERROR: cannot find atom for _NET_WM_STATE !\n" );
	return;
    }

    XClientMessageEvent xclient;
    memset( &xclient, 0, sizeof (xclient) );

    xclient.type = ClientMessage;
    xclient.window = win;
    xclient.message_type = wmNetWmState;
    xclient.format = 32;
    xclient.data.l[0] = _NET_WM_STATE_ADD;
    xclient.data.l[1] = wmStateAbove;
    xclient.data.l[2] = 0;
    xclient.data.l[3] = 0;
    xclient.data.l[4] = 0;

    XSendEvent(disp,
	       DefaultRootWindow(disp),
	       False,
	       SubstructureRedirectMask | SubstructureNotifyMask,
	       (XEvent *)&xclient );
}

XFontSet GetFontSet(void)
{
    char **missing_charsets;
    int count;
    char *def_str;
    static XFontSet fontset = None;

    if (fontset == None) {
	fontset = XCreateFontSet(disp,
				 // "-adobe-times-medium-r-*-*-14-*-*-*-*-*-iso8859-1,r14,k14",
				 // "-ricoh-*-bold-r-normal--24-*-*-*-*-iso10646-1",
				 "-sony-fixed-medium-r-*-*-24-*-*-*-*-*-*-*,"
				 "-*-fixed-medium-r-*-*-24-*-*-*-*-*-*-*",
				 &missing_charsets, &count, &def_str);
    }

    return fontset;
}

void Draw(void)
{
    puts("DRAW");
    // シェイプマスクの設定。(これってリサイズごとにするの？)
    {
	Pixmap shape_mask;
	static unsigned int _width = 0, _height = 0;
	unsigned int width, height;
	unsigned int dummy;
	Window root;
	int idummy;

	XGetGeometry(disp, win, &root, &idummy, &idummy,
		     &width, &height, &dummy, &dummy);
	// fprintf(stderr, "%dx%d\n", width, height);

	if (_width != width || _height != height) {
	    _width = width;
	    _height = height;

	    shape_mask = XCreatePixmap(disp, win, width, height, 1); // 二値pixmapを作る。
	    GC gc = XCreateGC(disp, shape_mask, 0, NULL);
	    XSetForeground(disp, gc, 0);
	    XFillRectangle(disp, shape_mask, gc,
			   0, 0, width, height);

	    DrawText(shape_mask, 1, True);

	    XShapeCombineMask(disp, win, ShapeBounding, 0, 0, shape_mask, ShapeSet);
	    XFreePixmap(disp, shape_mask);
	    XFreeGC(disp, gc);
	}
    }

    Colormap cm = DefaultColormap(disp, DefaultScreen(disp));
    XColor foreground;
    
    XParseColor(disp, cm, "#FF8000", &foreground);
    XAllocColor(disp, cm, &foreground);

    DrawText(win, foreground.pixel, False);
    // puts("Draw");
}

void HandleDragging(unsigned int button,
		    int start_x,
		    int start_y,
		    int xoff, int yoff)
{
    static Cursor cursor = None;

    if (cursor == None)
	cursor = XCreateFontCursor(disp, XC_hand2);

    XGrabPointer(disp, win, False,
		 PointerMotionHintMask | ButtonMotionMask | ButtonReleaseMask |
		 OwnerGrabButtonMask | Expose, GrabModeAsync, GrabModeAsync, None,
		 cursor, CurrentTime);

    XEvent event;

    while (1) {
	XNextEvent(disp, &event);

	switch (event.type) {
	case ButtonRelease:
	    XUngrabPointer(disp, CurrentTime);
	    return;
	case MotionNotify:
	    {
		Window root, child;
		int x, y, dummy;
		unsigned int mask;

		XQueryPointer(disp, DefaultRootWindow(disp), &root, &child, &x, &y, &dummy, &dummy, &mask);
		printf("(%d, %d)\n", x, y);
		XMoveWindow(disp, win, x - xoff, y - yoff);
	    }
	    break;
	case Expose:
	    Draw();
	    break;
	default:
	    ;
	}
    }
}

void EventLoop(void)
{
    XEvent event;

    while (1) {
	XNextEvent(disp, &event);
	switch (event.type) {
	case Expose:
	    Draw();
	    break;
	case ClientMessage:
	    // WM_DELETE_WINDOW を受け取った。
	    return;
	case ButtonPress:
	    HandleDragging( event.xbutton.button, 
			    event.xbutton.x_root,
			    event.xbutton.y_root,
			    event.xbutton.x,
			    event.xbutton.y);
	    break;
	default:
	    puts("other event");
	    break;
	}
    }
}

int main(int argc, char *argv[])
{
    if (argc != 1) {
	fprintf(stderr, "Usage: xsuper\n");
	exit (1);
    }

    Initialize();

    Draw();

    EventLoop();

    return 0;
}
