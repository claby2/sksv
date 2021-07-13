#include <X11/X.h>
#include <X11/XKBlib.h>
#include <X11/Xft/Xft.h>
#include <X11/Xutil.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

#define ARG(a, b) !strcmp(a, b)

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define INTERSECT(x, y, w, h, r) \
  (MAX(0, MIN((x) + (w), (r).x_org + (r).width) - MAX((x), (r).x_org)))

struct Key {
  char *name;
  int w;
  struct Key *next;
};

typedef struct {
  int x, y;
  unsigned int w, h;
} Geometry;

typedef struct {
  Display *dpy;
  Drawable buf;
  Window win;
  XftDraw *draw;
  int scr;
  Geometry geom;
} XWindow;

typedef struct {
  GC gc;
  XftColor color;
  XftFont *font;
} DC;

static Window root;
static XWindow xw;
static DC dc;

static char *font = "";
static char *usergeom = NULL;

static const double HEIGHT_SCALE_FACTOR = 1.5;
static const unsigned int KEYS_RETURN_SIZE = 32;
static const long int DELAY = 5000000;

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

void setwingeometry(Geometry *geom) {
  XWindowAttributes wa;
#ifdef XINERAMA
  Window dw;
  XineramaScreenInfo *info;
  int n, di, i;
  int px, py;
  unsigned int du;
  if ((info = XineramaQueryScreens(xw.dpy, &n))) {
    XQueryPointer(xw.dpy, root, &dw, &dw, &px, &py, &di, &di, &du);
    for (i = 0; i < n; i++)
      if (INTERSECT(px, py, 1, 1, info[i])) break;
    geom->x = info[i].x_org;
    geom->y = info[i].y_org;
    geom->w = info[i].width;
    XFree(info);
  } else
#endif
  {
    XGetWindowAttributes(xw.dpy, root, &wa);
    geom->x = geom->y = 0;
    geom->w = wa.width;
  }
  geom->h = dc.font->height * HEIGHT_SCALE_FACTOR;
}

void setup(void) {
  Colormap cm;
  Visual *vis;
  XGCValues gcv;
  XRenderColor rc;
  XSetWindowAttributes swa;
  XWindowAttributes wa;

  if (!(xw.dpy = XOpenDisplay(NULL))) die("Can't open display\n");
  xw.scr = XDefaultScreen(xw.dpy);
  root = XRootWindow(xw.dpy, xw.scr);
  dc.font = XftFontOpenName(xw.dpy, xw.scr, font);
  if (usergeom)
    XParseGeometry(usergeom, &xw.geom.x, &xw.geom.y, &xw.geom.w, &xw.geom.h);
  else
    setwingeometry(&xw.geom);
  swa.override_redirect = True;
  xw.win =
      XCreateWindow(xw.dpy, root, xw.geom.x, xw.geom.y, xw.geom.w, xw.geom.h, 0,
                    CopyFromParent, CopyFromParent, CopyFromParent,
                    CWOverrideRedirect | CWBackPixel, &swa);
  settitle("sksv");
  XMapRaised(xw.dpy, xw.win);
  XGetWindowAttributes(xw.dpy, xw.win, &wa);
  xw.buf = XCreatePixmap(xw.dpy, xw.win, xw.geom.w, xw.geom.h, wa.depth);

  gcv.graphics_exposures = 0;
  dc.gc = XCreateGC(xw.dpy, xw.win,
                    GCForeground | GCBackground | GCGraphicsExposures, &gcv);
  vis = XDefaultVisual(xw.dpy, xw.scr);
  cm = XDefaultColormap(xw.dpy, xw.scr);
  xw.draw = XftDrawCreate(xw.dpy, xw.buf, vis, cm);
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
    if (total_w > xw.geom.w) {
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
  // Malloc +2 to accommodate null terminator and additional space character.
  name = malloc(strlen(str) + 2);
  strcpy(name, str);
  strcat(name, " ");
  XftTextExtentsUtf8(xw.dpy, dc.font, (XftChar8 *)name, strlen(name), &ext);
  key->name = name;
  key->w = ext.xOff;
  return key;
}

void run(void) {
  char *text;
  char kr[KEYS_RETURN_SIZE];
  char pkr[KEYS_RETURN_SIZE];
  int change;
  int i;
  int keycode;
  struct Key *cur;
  struct Key *head = NULL;
  struct timespec ts = {.tv_sec = 0, .tv_nsec = DELAY};
  unsigned long keysym;

  memset(pkr, 0, KEYS_RETURN_SIZE * sizeof(char));
  for (;;) {
    nanosleep(&ts, NULL);
    change = 0;
    XQueryKeymap(xw.dpy, kr);
    for (i = 0; i < KEYS_RETURN_SIZE; i++) {
      if (kr[i] && kr[i] != pkr[i]) {
        keycode = i * 8 + log2(abs(kr[i]) ^ pkr[i]);
        // TODO: Use appropriate values for group and level arguments.
        keysym = XkbKeycodeToKeysym(xw.dpy, keycode, 0, 0);
        cur = keysymtokey(keysym);
        if (cur) {
          cur->next = head;
          head = cur;
          change = 1;
        }
      }
      pkr[i] = kr[i];
    }
    if (change) {
      XClearWindow(xw.dpy, xw.win);
      text = gettext(head);
      XftDrawString8(xw.draw, &dc.color, dc.font, 0, dc.font->height,
                     (XftChar8 *)text, strlen(text));
      XCopyArea(xw.dpy, xw.buf, xw.win, dc.gc, 0, 0, xw.geom.w, xw.geom.h, 0,
                0);
      XSetForeground(xw.dpy, dc.gc, XBlackPixel(xw.dpy, xw.scr));
      XFillRectangle(xw.dpy, xw.buf, dc.gc, 0, 0, xw.geom.w, xw.geom.h);
      XFlush(xw.dpy);
      free(text);
    }
  }
  // Free linked list.
  for (cur = head; cur != NULL; cur = cur->next) {
    free(cur->name);
    free(cur);
  }
}

void cleanup(void) {
  XftDrawDestroy(xw.draw);
  XftColorFree(xw.dpy, XDefaultVisual(xw.dpy, xw.scr),
               DefaultColormap(xw.dpy, xw.scr), &dc.color);
  XftFontClose(xw.dpy, dc.font);
  XFreePixmap(xw.dpy, xw.buf);
  XFreeGC(xw.dpy, dc.gc);
  XDestroyWindow(xw.dpy, xw.win);
  XCloseDisplay(xw.dpy);
  exit(EXIT_SUCCESS);
}

void usage(void) {
  fputs("usage: sksv [-fn font] [-g geometry]\n", stderr);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  int i;

  for (i = 1; i < argc; i++) {
    if (ARG(argv[i], "-fn"))
      font = argv[++i];
    else if (ARG(argv[i], "-g"))
      usergeom = argv[++i];
    else
      usage();
  }
  setup();
  run();
  cleanup();
}
