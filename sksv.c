#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <X11/X.h>
#include <X11/XKBlib.h>
#include <X11/Xutil.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

// TODO: Set default screen dimensions to reasonable values.
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

struct Key {
  char *name;
  int w;
  struct Key *next;
};

static Display *dpy;

static SDL_Window *win;
static SDL_Renderer *renderer;
static TTF_Font *font;
static const SDL_Color fg = {.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF};
static const SDL_Color bg = {.r = 0x00, .g = 0x00, .b = 0x00, .a = 0xFF};

static const unsigned int KEYS_RETURN_SIZE = 32;

void die(const char *errstr, ...) {
  va_list ap;

  va_start(ap, errstr);
  vfprintf(stderr, errstr, ap);
  va_end(ap);
  exit(EXIT_FAILURE);
}

void scc(int code) {
  if (code < 0) {
    die("SDL ERROR: %s\n", SDL_GetError());
  }
}

void *scp(void *ptr) {
  if (ptr == NULL) {
    die("SDL ERROR(ptr): %s\n", SDL_GetError());
  }
  return ptr;
}

void set_color(const SDL_Color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void setup(void) {
  if (!(dpy = XOpenDisplay(NULL))) die("Can't open display\n");
  scc(SDL_Init(SDL_INIT_VIDEO));
  scc(SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"));
  scc(TTF_Init());
  win = scp(SDL_CreateWindow("sksv", SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                             SCREEN_HEIGHT, SDL_WINDOW_ALWAYS_ON_TOP));
  renderer = scp(SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED));
  // TODO: Fetch font file and ptsize from user config.
  font = scp(TTF_OpenFont("/usr/share/fonts/TTF/FiraCode-Regular.ttf", 20));
}

int hasquit(void) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      return 1;
    }
  }
  return 0;
}

void renderkeys(struct Key *head) {
  SDL_Rect rect;
  SDL_Surface *surf;
  SDL_Texture *texture;
  char *text;
  int first = 0;
  int len = 0, total_w = 0;
  int w, h;
  struct Key *cur;
  struct Key *prev = NULL;

  for (cur = head; cur != NULL; cur = cur->next) {
    if (total_w > SCREEN_WIDTH) {
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
  surf = scp(TTF_RenderText_Solid(font, text, fg));
  texture = scp(SDL_CreateTextureFromSurface(renderer, surf));
  scc(TTF_SizeText(font, text, &w, &h));
  rect.x = 0;
  rect.y = 0;
  rect.w = total_w;
  rect.h = h;
  scc(SDL_RenderCopy(renderer, texture, &rect, &rect));
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surf);
  free(text);
}

struct Key *keysymtokey(unsigned long keysym) {
  struct Key *key;
  char *str;
  char *name;
  int w, h;

  str = XKeysymToString(keysym);
  if (!str) return NULL;
  key = (struct Key *)malloc(sizeof(struct Key));
  name = malloc(strlen(str) + 1);
  strcpy(name, str);
  strcat(name, " ");
  scc(TTF_SizeText(font, name, &w, &h));
  key->name = name;
  key->w = w;
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
    if (hasquit()) break;
    set_color(bg);
    SDL_RenderClear(renderer);
    XQueryKeymap(dpy, kr);
    for (int i = 0; i < KEYS_RETURN_SIZE; i++) {
      if (kr[i] && kr[i] != pkr[i]) {
        keycode = i * 8 + log2(abs(kr[i]) ^ pkr[i]);
        // TODO: Use appropriate values for group and level arguments.
        keysym = XkbKeycodeToKeysym(dpy, keycode, 0, 0);
        cur = keysymtokey(keysym);
        if (cur) {
          cur->next = head;
          head = cur;
        }
      }
      pkr[i] = kr[i];
    }
    if (head) renderkeys(head);
    SDL_RenderPresent(renderer);
  }
}

void cleanup(void) {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(win);
  SDL_Quit();
  XCloseDisplay(dpy);
  exit(EXIT_SUCCESS);
}

int main(void) {
  setup();
  run();
  cleanup();
}
