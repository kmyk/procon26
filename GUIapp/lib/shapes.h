#include <SDL.h>

#ifdef __MINGW32__
char *strdup( const char *s );
int strcasecmp( const char *s1, const char *s2 );
#endif

struct Render {
  SDL_Renderer *render;
  SDL_Surface *surf;
  Render();
  Render(SDL_Renderer*);
  void reset();
};

SDL_Color sdl_col(Uint32);
Uint32 int_col(SDL_Color);
void circleColor(Render,int xc, int yc, int r,Uint32 col);
void arcColor(Render,int xc,int yc,int radius,int stangle,int endangle,Uint32 color);
void polygonColor(Render,const short *xc,const short *yc,int n,Uint32 col);
void trigonColor(Render, int x1, int y1, int x2, int y2, int x3, int y3, Uint32 col);
void rectangleColor(Render,int x1,int y1,int x2,int y2,Uint32 col);
void hlineColor(Render,int x1,int x2,int y,Uint32 col);
void vlineColor(Render,int x,int y1,int y2,Uint32 col);
void lineColor(Render,int x1,int y1,int x2,int y2,Uint32 col);
void filledPolygonColor(Render, const Sint16 * vx, const Sint16 * vy, int n, Uint32 col);
void filledTrigonColor(Render, int x1, int y1, int x2, int y2, int x3, int y3, Uint32 col);
void boxColor(Render,int x1,int y1,int x2,int y2,Uint32 col);
void filledEllipseColor(Render,int xc, int yc, int rx, int ry,Uint32 col);
void filledCircleColor(Render,int xc, int yc, int rx,Uint32 col);
