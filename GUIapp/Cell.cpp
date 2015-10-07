#include <SDL2_gfxPrimitives.h>
#include "Cell.hpp"

Cell::Cell(int _x,int _y){
  x = _x;
  y = _y;
}
void Cell::set_pos(int _x,int _y){
  x = _x;
  y = _y;
}

void Cell::draw(SDL_Renderer* renderer){
    rectangleRGBA(renderer,x * size,y * size,(x + 1) * size,(y + 1) * size,
                  0,0,0,255);
    if(state == STATE_EMPTY){
      boxRGBA(renderer,x * size + edge,y * size + edge,(x + 1) * size - edge,(y + 1) * size - edge,
              255,255,255,255);
    }else{
      boxRGBA(renderer,x * size + edge,y * size + edge,(x + 1) * size - edge,(y + 1) * size - edge,
              0,0,0,255);
    }

}
