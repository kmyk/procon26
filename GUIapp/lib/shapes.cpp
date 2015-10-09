/* 
   Inspired by libgraph-1 by Faraz Shahbazker
   and by SDL_gfx-2 by Andreas Schiffler,
   both GPL licenced.
*/
#ifdef __MINGW32__
#include <math.h>
#endif
#include "shapes.h"

SDL_Color sdl_col(Uint32 c) {
  SDL_Color sdl_c;
  sdl_c.r=c>>24 & 0xff;
  sdl_c.g=c>>16 & 0xff;
  sdl_c.b=c>>8 & 0xff;
  sdl_c.a=0;
  return sdl_c;
}

Uint32 int_col(SDL_Color c) {
  return (c.r<<24) + (c.g<<16) + (c.b<<8) + 0xff;
}

static void mapsympixel(Render rend,int x, int y, int xc, int yc, int symnum) {
  switch(symnum){
  case 8:	// symmetric in all octents
    SDL_RenderDrawPoint(rend.render,y+xc, x+yc);
    SDL_RenderDrawPoint(rend.render,y+xc, -x+yc);
    SDL_RenderDrawPoint(rend.render,-y+xc, x+yc);
    SDL_RenderDrawPoint(rend.render,-y+xc, -x+yc);	   
  case 4 :	// symmetric in all quadrants
    SDL_RenderDrawPoint(rend.render,-x+xc, -y+yc);
    SDL_RenderDrawPoint(rend.render,-x+xc, y+yc);
  case 2 :	// symmetric about only X-axis
    SDL_RenderDrawPoint(rend.render,x+xc, -y+yc);
    SDL_RenderDrawPoint(rend.render,x+xc, y+yc);
    break;
  case 1 :	// symmetric about only Y-axis
    SDL_RenderDrawPoint(rend.render,-x+xc, y+yc);
    SDL_RenderDrawPoint(rend.render,x+xc, y+yc);        	   
  }
}   

static void set(Render rend,Uint32 col) {
  SDL_Color sdlc=sdl_col(col);
  SDL_SetRenderDrawColor(rend.render,sdlc.r,sdlc.g,sdlc.b,0xff);
}

void set(SDL_Rect &r,int x,int y,int w,int h) {
  r.x=x; r.y=y; r.w=w; r.h=h;
}

void arcColor(Render rend,int xc,int yc,int radius,int stangle,int endangle,Uint32 col) {
  float xold, yold, xnew, ynew;
  float sintheta, costheta, theta;
  int num;
  set(rend,col);

  theta = fmax(1./radius, 0.001);
  num = rint(M_PI * abs (endangle-stangle) / 180 / theta);

  sintheta = sinf(theta);
  costheta = cosf(theta);

  xold = radius * cosf(M_PI * stangle / 180);
  yold = radius * sinf(M_PI * stangle / 180);

  for (; num; num--) {
    xnew = xold * costheta - yold * sintheta;
    ynew = xold * sintheta + yold * costheta;
    SDL_RenderDrawPoint(rend.render,rint(xnew+xc),rint(ynew+yc));
    xold = xnew; yold = ynew;
  }
}

void rectangleColor(Render rend,int x1,int y1,int x2,int y2,Uint32 col) {
  set(rend,col);
  SDL_Rect r;
  set(r,x1,y1,x2-x1+1,y2-y1+1); // 1 extra, RenderDrawRect draws 1 less
  SDL_RenderDrawRect(rend.render,&r);
}
void boxColor(Render rend,int x1,int y1,int x2,int y2,Uint32 col) {
  set(rend,col);
  SDL_Rect r;
  set(r,x1,y1,x2-x1,y2-y1);
  SDL_RenderFillRect(rend.render,&r);
}
void lineColor(Render rend,int x1,int y1,int x2,int y2,Uint32 col) {
  set(rend,col);
  SDL_RenderDrawLine(rend.render,x1,y1,x2,y2);
}
void hlineColor(Render rend,int x1,int x2,int y,Uint32 col) {
  set(rend,col);
  SDL_RenderDrawLine(rend.render,x1,y,x2,y);
}
void vlineColor(Render rend,int x,int y1,int y2,Uint32 col) {
  set(rend,col);
  SDL_RenderDrawLine(rend.render,x,y1,x,y2);
}

void polygonColor(Render rend,const short *vx,const short *vy,int n,Uint32 col) {
  set(rend,col);
  const short *x1, *y1, *x2, *y2;
  x1 = x2 = vx;
  y1 = y2 = vy;
  x2++;
  y2++;
  for (int i = 1; i < n; i++) {
    SDL_RenderDrawLine(rend.render,*x1,*y1,*x2,*y2);
    x1 = x2;
    y1 = y2;
    x2++;
    y2++;
  }
  SDL_RenderDrawLine(rend.render,*x1,*y1,*vx,*vy);
}

int compareInt(const void *a, const void *b) {
  return (*(const int *) a) - (*(const int *) b);
}

void filledPolygonColor(Render rend, const Sint16 * vx, const Sint16 * vy, int n, Uint32 col) {
  set(rend,col);
  int y, xa, xb,
      miny, maxy,
      x1, y1,
      x2, y2,
      ind1, ind2,
      ints;
  const int int_dim=10;
  int polyInts[int_dim];
  miny = vy[0];
  maxy = vy[0];
  for (int i = 1; i < n; i++) {
  if (vy[i] < miny) {
      miny = vy[i];
    } else if (vy[i] > maxy) {
        maxy = vy[i];
    }
  }
  for (y = miny; y <= maxy; y++) {
    ints = 0;
    for (int i = 0; i < n; i++) {
      if (!i) {
        ind1 = n - 1;
        ind2 = 0;
      } else {
        ind1 = i - 1;
        ind2 = i;
      }
      y1 = vy[ind1];
      y2 = vy[ind2];
      if (y1 < y2) {
        x1 = vx[ind1];
        x2 = vx[ind2];
      } else if (y1 > y2) {
        y2 = vy[ind1];
        y1 = vy[ind2];
        x2 = vx[ind1];
        x1 = vx[ind2];
      } else
        continue;
      if ( (y >= y1 && y < y2) || (y == maxy && y > y1 && y <= y2) ) {
        polyInts[ints++] = ((65536 * (y - y1)) / (y2 - y1)) * (x2 - x1) + (65536 * x1);
        if (ints==int_dim) { puts("filledPolygonColor: too much points"); return; }
      }         
  }
  qsort(polyInts, ints, sizeof(int), compareInt);

  for (int i = 0; i < ints; i += 2) {
      xa = polyInts[i] + 1;
      xa = (xa >> 16) + ((xa & 32768) >> 15);
      xb = polyInts[i+1] - 1;
      xb = (xb >> 16) + ((xb & 32768) >> 15);
      SDL_RenderDrawLine(rend.render,xa,y,xb,y);
    }
  }
}
                                 
void filledTrigonColor(Render rend, int x1, int y1, int x2, int y2, int x3, int y3, Uint32 col) {
  Sint16 vx[3],
         vy[3];
  vx[0]=x1; vx[1]=x2; vx[2]=x3;
  vy[0]=y1; vy[1]=y2; vy[2]=y3;
  filledPolygonColor(rend,vx,vy,3,col);
}

void trigonColor(Render rend, int x1, int y1, int x2, int y2, int x3, int y3, Uint32 col) {
  Sint16 vx[3],
         vy[3];
  vx[0]=x1; vx[1]=x2; vx[2]=x3;
  vy[0]=y1; vy[1]=y2; vy[2]=y3;
  polygonColor(rend,vx,vy,3,col);
}

void circleColor(Render rend,int xc, int yc, int radius,Uint32 col) { // van: ~/libgraph-1.0.2/shapes.c
  set(rend,col);
  int x, y,
      p=1-radius;
   
  mapsympixel(rend,0, radius, xc, yc, 2);
  mapsympixel(rend,radius,0, xc, yc, 1);
  for(x=1, y=radius, p=3-radius; x<=y; x++) {
    mapsympixel(rend,x, y, xc, yc, 8);
    if (p>=0) {
      p += (2*x - 2*y + 1);
      y--;
    }
    else
      p += (2*x + 3);
  }
}

void filledCircleColor(Render rend,int xc, int yc, int r,Uint32 col) {
  set(rend,col);
  int x, y,
      p=1-r;
  SDL_RenderDrawLine(rend.render,xc,-r+yc,xc,r+yc);
  for(x=1, y=r, p=3-r; x<=y; x++) {
    SDL_RenderDrawLine(rend.render,x+xc,-y+yc,x+xc,y+yc);
    SDL_RenderDrawLine(rend.render,-x+xc,-y+yc,-x+xc,y+yc);
    SDL_RenderDrawLine(rend.render,-y+xc,-x+yc,-y+xc,x+yc);
    SDL_RenderDrawLine(rend.render,y+xc,-x+yc,y+xc,x+yc);
    if (p>=0) {
      p += (2*x - 2*y + 1);
      y--;
    }
    else
      p += (2*x + 3);
  }
}

void filledEllipseColor(Render rend,int xc, int yc, int rx, int ry,Uint32 col) {
  set(rend,col);
  int x, y;
  int p1, p2, rx2, ry2;
  rx2 = rx*rx;
  ry2 = ry*ry;
  x=0;
  y=ry;
  SDL_RenderDrawLine(rend.render,xc,-y+yc,xc,y+yc); // between topmost and bottom-most points
  p1= ry2-rx2*ry+rx2/4;

  do {
      x++;
      if(p1<0)
	p1+= 2*ry2*x+ry2;
      else {
	y--;
	p1+= 2*ry2*x-2*rx2*y+1+ry2;
      }
      SDL_RenderDrawLine(rend.render,x+xc,-y+yc,x+xc,y+yc);
      SDL_RenderDrawLine(rend.render,-x+xc,-y+yc,-x+xc,y+yc);
  } while(ry2*x<rx2*y);
  p2 = (ry2*(x+1/2)*(x+1/2) + rx2*(y-1)*(y-1) -rx2*ry2);
  do {
      y--;
      if (p2>0)
	p2+= -2*rx2*y + rx2;
      else {
	x++;
	p2+= 2*ry2*x - 2*rx2*y + rx2;
      }
      SDL_RenderDrawLine(rend.render,x+xc,-y+yc,x+xc,y+yc);
      SDL_RenderDrawLine(rend.render,-x+xc,-y+yc,-x+xc,y+yc);
  } while(y>=0);
}
