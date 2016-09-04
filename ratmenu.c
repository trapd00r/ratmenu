/*
 * ratmenu.c
 *
 * This program puts up a window that is just a menu, and executes
 * commands that correspond to the items selected.
 *
 * Initial idea: Arnold Robbins
 * Version using libXg: Matty Farrow (some ideas borrowed)
 * This code by: David Hogan and Arnold Robbins
 *
 * Copyright (c), Arnold Robbins and David Hogan
 *
 * Arnold Robbins
 * arnold@skeeve.atl.ga.us
 * October, 1994
 *
 * Code added to cause pop-up (unIconify) to move menu to mouse.
 * Christopher Platt
 * platt@coos.dartmouth.edu
 * May, 1995
 *
 * Said code moved to -teleport option, and -warp option added.
 * Arnold Robbins
 * June, 1995
 *
 * Code added to allow -fg and -bg colors.
 * John M. O'Donnell
 * odonnell@stpaul.lampf.lanl.gov
 * April, 1997
 *
 * Ratpoison windowmanager specific hacking; removed a lot of junk
 * and added keyboard functionality
 * Jonathan Walther
 * krooger@debian.org
 * September, 2001
 *
 * Provided initial implementation of -persist, inspiring
 * the version that actually made it into the code.
 * Rupert Levene 
 * r.levene@lancaster.ac.uk
 * October, 2002
 *
 * Provided a menu-method file to allow ratmenu to access the Debian
 * menu hierarchy.
 * Fabricio Matheus Goncalves
 * fmatheus@bigbross.com
 * April, 2003
 *
 * Provided Xresource handling code
 * Zrajm C Akfohg
 * zrajm@klingonska.org
 * March 2003
 */

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/keysym.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Xlocale.h>

char version[] = "@(#) ratmenu version 2.3";

#define FONT "9x15bold"
#define	MenuMask (ExposureMask|StructureNotifyMask|KeyPressMask)
#define CTL(c) (c - '`')
#define XTextWidth XmbTextEscapement
#define XLoadQueryFont XLoadQueryFontSet
XFontSet XLoadQueryFontSet(Display *, const char *);

Display *dpy;		/* lovely X stuff */
int screen;
Window root;
Window menuwin;
GC gc;
unsigned long fg;
unsigned long bg;
char *fgcname = NULL;
char *bgcname = NULL;
Colormap dcmap;
XColor color;
XFontSet font;
short font__ascent, font__descent;
Atom wm_protocols;
Atom wm_delete_window;
int g_argc;			/* for XSetWMProperties to use */
char **g_argv;
int savex, savey;
int dpyheight;
Window savewindow;


char *progname;		/* my name */
char *displayname;	/* X display */
char *fontname = NULL;  /* font */
char *labelname;	/* window and icon name */

char **labels;		/* list of labels and commands */
char **commands;
int numitems;
int startitem;          /* which item to highlight first */
int curitem;
int olditem;
int visible;            /* number of items visible on screen */
int off;                /* offset used by dreary */
enum {unset=0, left=1, yes=1, snazzy=1, center=2, no=2, dreary=2, right=3} align, persist, menustyle;

char *shell = "/bin/sh";	/* default shell */
char *prevmenu = NULL;          /* previous menu */

void ask_wm_for_delete(void);
void reap(int);
void redraw_snazzy(int, int, int, int);
void redraw_dreary(int, int, int, int);
void (*redraw) (int, int, int, int);
void run_menu();
void set_wm_hints(int, int);
void spawn(char*);
void usage(void);
void xresources(void);
void defaults(void);

XFontSet XLoadQueryFontSet(Display *disp, const char *fontset_name)
{
  XFontSet fontset;
  int  missing_charset_count;
  char **missing_charset_list;
  char *def_string;
  
  fontset = XCreateFontSet(disp, fontset_name,
                           &missing_charset_list, &missing_charset_count,
                           &def_string);
  if (missing_charset_count /* && debug */) {
    fprintf(stderr, "Missing charsets in FontSet(%s) creation.\n", fontset_name);
    XFreeStringList(missing_charset_list);
  }
  return fontset;
}


/* main --- crack arguments, set up X stuff, run the main menu loop */

int
main(int argc, char **argv)
{
	int i, j;
	char *cp;
	XGCValues gv;
	unsigned long mask;
	XFontSetExtents *extent;

	g_argc = argc;
	g_argv = argv;

	setlocale(LC_CTYPE, "");

	/* set default label name */
	if ((cp = strrchr(argv[0], '/')) == NULL)
		labelname = argv[0];
	else
		labelname = ++cp;

	/* and program name for diagnostics */
	progname = labelname;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-display") == 0) {
			displayname = argv[++i];
                } else if (strcmp(argv[i], "-font") == 0) {
                        fontname = argv[++i];
		} else if (strcmp(argv[i], "-label") == 0) {
			labelname = argv[++i];
		} else if (strcmp(argv[i], "-shell") == 0) {
			shell = argv[++i];
		} else if (strcmp(argv[i], "-back") == 0) {
			prevmenu = argv[++i];
		} else if (strcmp(argv[i], "-fg") == 0) {
			fgcname = argv[++i];
		} else if (strcmp(argv[i], "-bg") == 0) {
			bgcname = argv[++i];
		} else if (strcmp(argv[i], "-io") == 0) {
			startitem = atoi(argv[++i]) - 1;
		} else if (strcmp(argv[i], "-persist") == 0) {
			persist = yes;
		} else if (strcmp(argv[i], "-align") == 0) {
			if (strcmp(argv[i+1], "left") == 0) {
				align = left;
			} else if (strcmp(argv[i+1], "center") == 0) {
				align = center;
			} else if (strcmp(argv[i+1], "right") == 0) {
				align = right;
			} else {
				usage();
			}
			i++;
		} else if (strcmp(argv[i], "-style") == 0) {
			if (strcmp(argv[i+1], "snazzy") == 0) {
				menustyle = snazzy;
			} else if (strcmp(argv[i+1], "dreary") == 0) {
				menustyle = dreary;
			} else {
				usage();
			}
			i++;
		} else if (strcmp(argv[i], "-version") == 0) {
			printf("%s\n", version);
			exit(0);
		} else if (argv[i][0] == '-') {
			usage();
		} else {
			break;
		}
	}

	if ((argc-i) % 2 == 1) /* every menu item needs a label AND command */
		usage();

	if ((argc-i) * 2 <= 0)
		usage();

	numitems = (argc-i) / 2;

	labels   = (char **) malloc(numitems * sizeof(char *));
	commands = (char **) malloc(numitems * sizeof(char *));

	if (commands == NULL || labels == NULL) {
		fprintf(stderr, "%s: no memory!\n", progname);
		exit(1);
	}

	for (j=0; i < argc; j++) {
		labels[j]   = argv[i++];
		commands[j] = argv[i++];
	}

	dpy = XOpenDisplay(displayname);
	if (dpy == NULL) {
		fprintf(stderr, "%s: cannot open display", progname);
		if (displayname != NULL)
			fprintf(stderr, " %s", displayname);
		fprintf(stderr, "\n");
		exit(1);
	}
	screen = DefaultScreen(dpy);
	dcmap = DefaultColormap (dpy, screen);

	xresources();
	defaults();

	if ((font = XLoadQueryFont(dpy, fontname)) == NULL) {
		fprintf(stderr, "%s: fatal: cannot load font %s\n", progname, fontname);
		exit(1);
	}
	extent = XExtentsOfFontSet(font);
	font__ascent = extent->max_logical_extent.height * 4 / 5;
	font__descent = extent->max_logical_extent.height / 5;

	gv.foreground = fg^bg;
	gv.background = bg;
	gv.function = GXxor;
	gv.line_width = 0;
	mask = GCForeground | GCBackground | GCFunction | GCLineWidth;

	root = RootWindow(dpy, screen);
	gc = XCreateGC(dpy, root, mask, &gv);

	dpyheight = DisplayHeight(dpy, screen);

	signal(SIGCHLD, reap);

	run_menu();

	XCloseDisplay(dpy);
	exit(0);
}

/* spawn --- run a command */

void
spawn(char *com)
{
	char *sh_basename;

	if ((sh_basename = strrchr(shell, '/')) != NULL) {
		sh_basename++;
	} else {
		sh_basename = shell;
	}

	XCloseDisplay(dpy);
	execl(shell, sh_basename, "-c", com, NULL);
	execl("/bin/sh", "sh", "-c", com, NULL);
	exit(1);
}

/* reap --- collect dead children */

void
reap(int s)
{
	(void) wait((int *) NULL);
	signal(s, reap);
}

/* usage --- print a usage message and die */

void
usage(void)
{
	fprintf(stderr, "usage: %s [-display displayname] [-style {snazzy|dreary}]\n"
			"[-shell shell] [-label name] [-align {left|center|right}]\n"
			"[-fg fgcolor] [-bg bgcolor] [-font fname] [-back prevmenu]\n"
			"[-persist] [-version] [-io offset] [menuitem command] ...\n", progname);
	exit(0);
}

/* run_menu --- put up the window, execute selected commands */

void
run_menu(void)
{
	KeySym key;
	XEvent ev;
	XClientMessageEvent *cmsg;
	int i, wide, high, dx, dy;

	for (i = 0, dx = 0, wide = 0; i < numitems; i++) {
		wide = XTextWidth(font, labels[i], strlen(labels[i])) + 4;
		if (wide > dx)
			dx = wide;
	}
	wide = dx;

	high = font__ascent + font__descent + 1;
	visible = dpyheight / high;
	if (visible > numitems) { visible = numitems; }
	dy = visible * high;

	set_wm_hints(dx, dy);

	ask_wm_for_delete();


	XSelectInput(dpy, menuwin, MenuMask);

	XMapWindow(dpy, menuwin);

	if (startitem < 0) {
		curitem = 0;
	} else if (startitem >= numitems) {
		curitem = numitems - 1;
	} else {
		curitem = startitem;
	}

	if (menustyle == snazzy) {
		redraw = redraw_snazzy;
	} else if (menustyle == dreary)  {
		redraw = redraw_dreary;
		olditem = curitem;
	}

	off = 0;

	for (;;) {
		char keystr[10];
		int keystr_len;
		XNextEvent(dpy, &ev);
		switch (ev.type) {
		case KeyPress:
			keystr_len = XLookupString(&ev.xkey, keystr, sizeof(keystr), &key, NULL);
			if (keystr_len == 1) {
				/* construct some of the "easier" keybindings
				   by looking at the string representation of
				   what got hit, and faking one of the KeySyms
				   that we already handle.
				*/
				switch (keystr[0]) {
				case 'q':
				case '\033':
				case CTL('g'):
					key = XK_Escape;
					break;
				case CTL('p'):
					key = XK_Up;
					break;
				case CTL('n'):
				case CTL('i'): /* c-i is *like* tab */
					key = XK_Down;
					break;
				case CTL('m'):
				case CTL('j'):
				case CTL('f'):
					key = XK_Right;
					break;
				case CTL('b'):
					key = XK_Left;
					break;
				}
			}

			switch (key) {
			case XK_Escape: case XK_q:
				return;
				break;
			case XK_Left: case XK_h:
				if (prevmenu) {
					spawn(prevmenu);
				}
				break;
			case XK_Right: case XK_Return: case XK_l:
				spawn(commands[curitem]);
				break;
			case XK_Tab: case XK_space: case XK_Down: case XK_j: case XK_plus:
				++curitem >= numitems ? curitem =            0 : 0 ;
				redraw(curitem, high, wide, 0);
				break;
			case XK_BackSpace: case XK_Up: case XK_k: case XK_minus:
				curitem-- <= 0        ? curitem = numitems - 1 : 0 ;
				redraw(curitem, high, wide, 0);
				break;
			}
			break;
		case MapNotify: case Expose: redraw(curitem, high, wide, 1); break;
		case UnmapNotify: if (persist == no) return; else XClearWindow(dpy, menuwin); break;
		case CirculateNotify: if (persist == no) return; break;
		case ClientMessage:
			cmsg = &ev.xclient;
			if (cmsg->message_type == wm_protocols
				&& cmsg->data.l[0] == wm_delete_window)
				return;
		}
	}
}

/* set_wm_hints --- set all the window manager hints */

void
set_wm_hints(int wide, int high)
{
	XWMHints *wmhints;
	XSizeHints *sizehints;
	XClassHint *classhints;
	XTextProperty wname;

	if ((sizehints = XAllocSizeHints()) == NULL) {
		fprintf(stderr, "%s: could not allocate size hints\n",
			progname);
		exit(1);
	}

	if ((wmhints = XAllocWMHints()) == NULL) {
		fprintf(stderr, "%s: could not allocate window manager hints\n",
			progname);
		exit(1);
	}

	if ((classhints = XAllocClassHint()) == NULL) {
		fprintf(stderr, "%s: could not allocate class hints\n",
			progname);
		exit(1);
	}

	/* fill in hints in order to parse geometry spec */
	sizehints->width = sizehints->min_width = sizehints->max_width = wide;
	sizehints->height = sizehints->min_height = sizehints->max_height = high;
	sizehints->flags = USSize|PSize|PMinSize|PMaxSize;

	if (XStringListToTextProperty(& labelname, 1, & wname) == 0) {
		fprintf(stderr, "%s: could not allocate window name structure\n",
			progname);
		exit(1);
	}

	menuwin = XCreateSimpleWindow(dpy, root, sizehints->x, sizehints->y,
				sizehints->width, sizehints->height, 1, fg, bg);

	wmhints->input = True;
	wmhints->initial_state = NormalState;
	wmhints->flags = StateHint | InputHint;

	classhints->res_name = progname;
	classhints->res_class = "ratmenu";

	XSetWMProperties(dpy, menuwin, & wname, NULL,
		g_argv, g_argc, sizehints, wmhints, classhints);
}

/* ask_wm_for_delete --- jump through hoops to ask WM to delete us */

void
ask_wm_for_delete(void)
{
	int status;

	wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	status = XSetWMProtocols(dpy, menuwin, & wm_delete_window, 1);

	if (status != True)
		fprintf(stderr, "%s: could not ask for clean delete\n",
			progname);
}

/* redraw --- actually draw the menu */

void
redraw_snazzy (int curitem, int high, int wide, int fullredraw)
{
	int i, j, ty, tx;

	XClearWindow(dpy, menuwin);
	for (i = 0, j = curitem; i < visible; i++, j++) {
		j %= numitems;
		if (align == left) {
			tx = 0;
		} else if (align == center) {
                	tx = (wide - XTextWidth(font, labels[j], strlen(labels[j]))) / 2;
		} else {/* align == right */
                	tx = wide - XTextWidth(font, labels[j], strlen(labels[j]));
		}
                ty = i*high + font__ascent + 1;
                XmbDrawString(dpy, menuwin, font, gc, tx, ty, labels[j], strlen(labels[j]));
	}
	XFillRectangle(dpy, menuwin, gc, 0, 0, wide, high); 
}

void
redraw_dreary (int curitem, int high, int wide, int fullredraw)
{
	int i, ty, tx, oldoff;

	oldoff = off;
	if (curitem + 1 > visible + off) {
		off = curitem + 1 - visible;
	} else if (curitem + 1 <= off) {
		off = curitem;
	}
	if (fullredraw || oldoff != off) {
		XClearWindow(dpy, menuwin);
		for (i = 0; i < visible; i++) {
			if (align == left) {
				tx = 0;
			} else if (align == center) {
                		tx = (wide - XTextWidth(font, labels[i+off], strlen(labels[i+off]))) / 2;
			} else {/* align == right */
                		tx = wide - XTextWidth(font, labels[i+off], strlen(labels[i+off]));
			}
                	ty = i*high + font__ascent + 1;
                	XmbDrawString(dpy, menuwin, font, gc, tx, ty, labels[i+off], strlen(labels[i+off]));
		}
		XFillRectangle(dpy, menuwin, gc, 0, (curitem-off)*high, wide, high); 
	} else {
		XFillRectangle(dpy, menuwin, gc, 0, (olditem-off)*high, wide, high); 
		XFillRectangle(dpy, menuwin, gc, 0, (curitem-off)*high, wide, high); 
	}
	olditem = curitem;
}

void
xresources(void)
{
	char *res;
	
	if (bgcname == NULL) {
		if ((res = XGetDefault(dpy, progname, "bgcolor")) != NULL ) {
			bgcname = strdup(res);
		}
	}
	if (fgcname == NULL) {
		if ((res = XGetDefault(dpy, progname, "fgcolor")) != NULL ) {
			fgcname = strdup(res);
		}
	}
	if (fontname == NULL) {
		if ((res = XGetDefault(dpy, progname, "font")) != NULL ) {
			fontname = strdup(res);
		}
	}
	if (menustyle == unset) {
		if ((res = XGetDefault(dpy, progname, "style")) != NULL ) {
			if      (strcasecmp(res, "dreary") == 0) menustyle = dreary;
			else if (strcasecmp(res, "snazzy") == 0) menustyle = snazzy;
		}
	}
	if (persist == unset) {
		if ((res = XGetDefault(dpy, progname, "persist")) != NULL ) {
			if      (strcasecmp(res, "on"  )  == 0) persist = yes;
			else if (strcasecmp(res, "off")   == 0) persist = no;
			else if (strcasecmp(res, "yes")   == 0) persist = yes;
			else if (strcasecmp(res, "no")    == 0) persist = no;
			else if (strcasecmp(res, "true")  == 0) persist = yes;
			else if (strcasecmp(res, "false") == 0) persist = no;
		}
	}
	if (align == unset) {
		if ((res = XGetDefault(dpy, progname, "align")) != NULL ) {
			if      (strcasecmp(res, "left"  ) == 0) align = left;
			else if (strcasecmp(res, "center") == 0) align = center;
			else if (strcasecmp(res, "right" ) == 0) align = right;
		}
	}
}

void
defaults(void)
{
	if (bgcname == NULL || XParseColor(dpy, dcmap, bgcname, &color) == 0 || XAllocColor(dpy, dcmap, &color) == 0) {
		bg = BlackPixel(dpy, screen);
	} else { bg = color.pixel; }
	if (fgcname == NULL || XParseColor(dpy, dcmap, fgcname, &color) == 0 || XAllocColor(dpy, dcmap, &color) == 0) {
		fg = WhitePixel(dpy, screen);
	} else { fg = color.pixel; }
	if (fontname == NULL) { fontname = FONT; }
	if (persist == unset) persist = no;
	if (align == unset) align = left;
	if (menustyle == unset) menustyle = snazzy;
}
