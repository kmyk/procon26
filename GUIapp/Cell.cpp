#include <SDL2_gfxPrimitives.h>
#include "Cell.hpp"
#include "Object.hpp"
#include "Map.hpp"

Cell::Cell(int _x,int _y){
  x = _x;
  y = _y;
}
void Cell::set_pos(int _x,int _y){
  x = _x;
  y = _y;
}

void Cell::draw(SDL_Renderer* renderer){
  int dx = Map::edge + x;
  int dy = Map::edge + y;
  boxRGBA(renderer,dx * size,dy * size,(dx + 1) * size,(dy + 1) * size,
          0,0,0,255);
  /*rectangleRGBA(renderer,dx * size,dy * size,(dx + 1) * size,(dy + 1) * size,
                255,255,255,255);*/

  if(state == STATE_EMPTY){
    boxRGBA(renderer,dx * size + edge,dy * size + edge,(dx + 1) * size - edge,(dy + 1) * size - edge,
            255,255,255,255);
  }else if(state == STATE_FILL){
    boxRGBA(renderer,dx * size + edge,dy * size + edge,(dx + 1) * size - edge,(dy + 1) * size - edge,
            0,0,0,255);
  }else if(state == STATE_EMPTY_P){
    boxRGBA(renderer,dx * size + edge,dy * size + edge,(dx + 1) * size - edge,(dy + 1) * size - edge,
            192,192,192,255);
  }else if(state == STATE_FILL_P){
    boxRGBA(renderer,dx * size + edge,dy * size + edge,(dx + 1) * size - edge,(dy + 1) * size - edge,
            128,128,128,255);
  }
}
