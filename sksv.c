#include <X11/X.h>
#include <X11/XKBlib.h>
#include <X11/Xft/Xft.h>
#include <X11/Xutil.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

// TODO: Set default screen dimensions to reasonable values.
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

struct Key {
  char *name;
  int w;
  struct Key *next;
};

typedef struct {
  Display *dpy;
  Drawable buf;
  Window win;
  XftDraw *draw;
  int scr;
} XWindow;

typedef struct {
  GC gc;
  XftColor color;
  XftFont *font;
} DC;

static XWindow xw;
static DC dc;

static const unsigned int KEYS_RETURN_SIZE = 32;

void die(const char *errstr, ...) {
  va_list ap;

  va_start(ap, errstr);
  vfprintf(stderr, errstr, ap);
  va_end(ap);
  exit(EXIT_FAILURE);
}

void settitle(char *p) {
  XTextProperty prop;
  Xutf8TextListToTextProperty(xw.dpy, &p, 1, XUTF8StringStyle, &prop);
  XSetWMName(xw.dpy, xw.win, &prop);
  XFree(prop.value);
}

void setup(void) {
  Colormap cm;
  Visual *vis;
  XGCValues gcv;
  XRenderColor rc;
  XSetWindowAttributes swa;
  XWindowAttributes attr;

  if (!(xw.dpy = XOpenDisplay(NULL))) die("Can't open display\n");
  xw.scr = XDefaultScreen(xw.dpy);
  swa.override_redirect = True;
  xw.win =
      XCreateWindow(xw.dpy, XDefaultRootWindow(xw.dpy), 0, 0, WINDOW_WIDTH,
                    WINDOW_HEIGHT, 0, CopyFromParent, CopyFromParent,
                    CopyFromParent, CWOverrideRedirect | CWBackPixel, &swa);
  settitle("sksv");
  XMapRaised(xw.dpy, xw.win);
  XGetWindowAttributes(xw.dpy, xw.win, &attr);
  xw.buf =
      XCreatePixmap(xw.dpy, xw.win, WINDOW_WIDTH, WINDOW_HEIGHT, attr.depth);

  gcv.graphics_exposures = 0;
  dc.gc = XCreateGC(xw.dpy, xw.win,
                    GCForeground | GCBackground | GCGraphicsExposures, &gcv);
  vis = XDefaultVisual(xw.dpy, xw.scr);
  cm = XDefaultColormap(xw.dpy, xw.scr);
  xw.draw = XftDrawCreate(xw.dpy, xw.buf, vis, cm);
  // TODO: Source font name from config.
  dc.font = XftFontOpenName(xw.dpy, xw.scr, "Fira Code:size=20");
  rc.alpha = rc.red = rc.green = rc.blue = 0xFFFF;
  XftColorAllocValue(xw.dpy, vis, cm, &rc, &dc.color);
}

char *gettext(struct Key *head) {
  char *text;
  int first = 0;
  int len = 0, total_w = 0;
  struct Key *cur;
  struct Key *prev = NULL;

  for (cur = head; cur != NULL; cur = cur->next) {
    if (total_w > WINDOW_WIDTH) {
      if (prev) prev->next = NULL;
      free(cur);
    } else {
      total_w += cur->w;
      len += strlen(cur->name);
      prev = cur;
    }
  }
  text = malloc(len + 1);
  for (cur = head; cur != NULL; cur = cur->next) {
    if (first == 0) {
      strcpy(text, cur->name);
      first = 1;
    } else
      strcat(text, cur->name);
  }
  return text;
}

struct Key *keysymtokey(unsigned long keysym) {
  XGlyphInfo ext;
  char *name;
  char *str;
  struct Key *key;

  str = XKeysymToString(keysym);
  if (!str) return NULL;
  key = (struct Key *)malloc(sizeof(struct Key));
  name = malloc(strlen(str) + 1);
  strcpy(name, str);
  strcat(name, " ");
  XftTextExtents32(xw.dpy, dc.font, (FcChar32 *)name, strlen(name), &ext);
  key->name = name;
  key->w = ext.width;
  return key;
}

void run(void) {
  char kr[KEYS_RETURN_SIZE];
  char pkr[KEYS_RETURN_SIZE];
  int keycode;
  struct Key *cur;
  struct Key *head = NULL;
  unsigned long keysym;

  for (;;) {
    XQueryKeymap(xw.dpy, kr);
    for (int i = 0; i < KEYS_RETURN_SIZE; i++) {
      if (kr[i] && kr[i] != pkr[i]) {
        keycode = i * 8 + log2(abs(kr[i]) ^ pkr[i]);
        // TODO: Use appropriate values for group and level arguments.
        keysym = XkbKeycodeToKeysym(xw.dpy, keycode, 0, 0);
        cur = keysymtokey(keysym);
        if (cur) {
          cur->next = head;
          head = cur;
          XClearWindow(xw.dpy, xw.win);
          char *text = gettext(head);
          XftDrawString8(xw.draw, &dc.color, dc.font, 0,
                         dc.font->ascent + dc.font->descent, (XftChar8 *)text,
                         strlen(text));
          XCopyArea(xw.dpy, xw.buf, xw.win, dc.gc, 0, 0, WINDOW_WIDTH,
                    WINDOW_HEIGHT, 0, 0);
          XFillRectangle(xw.dpy, xw.buf, dc.gc, 0, 0, WINDOW_WIDTH,
                         WINDOW_HEIGHT);
          XFlush(xw.dpy);
          free(text);
        }
      }
      pkr[i] = kr[i];
    }
  }
}

void cleanup(void) {
  XftDrawDestroy(xw.draw);
  XftColorFree(xw.dpy, XDefaultVisual(xw.dpy, xw.scr),
               DefaultColormap(xw.dpy, xw.scr), &dc.color);
  XftFontClose(xw.dpy, dc.font);
  XFreeGC(xw.dpy, dc.gc);
  XDestroyWindow(xw.dpy, xw.win);
  XCloseDisplay(xw.dpy);
  exit(EXIT_SUCCESS);
}

int main(void) {
  setup();
  run();
  cleanup();
}
