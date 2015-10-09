#include <SDL2_gfxPrimitives.h>
#include "Stone.hpp"
#include "Cell.hpp"

Stone::Stone(){
  pX = 0;
  pY = 0;
  state = STATE_NONE;
  geometry = new bool*[row];
  for(int y = 0; y < row; y++){
      geometry[y] = new bool[colum];
      for(int x = 0; x < colum; x++){
        geometry[y][x] = false;
      }
  }
}

Stone::~Stone(){
  for(int y = 0; y < row; y++){
      delete [] geometry[y];
  }
  delete[] geometry;

}
void Stone::set_preview_pos(int _x,int _y){
  pX = _x;
  pY =_y;
}
void Stone::set_stone_state(SState _state){
    state = _state;
}
Stone::SState Stone::get_stone_state(){
  return state;
}
void Stone::preview(SDL_Renderer* renderer){
  for(int y = 0; y < row; y++){
    for(int x = 0; x < colum; x++){
      int sx = pX + Cell::size * x;
      int sy = pY + Cell::size * y;
      rectangleRGBA(renderer,sx,sy,sx + Cell::size,sy + Cell::size,
                    0,0,0,255);
      if(geometry[y][x]){
        boxRGBA(renderer,sx + Cell::edge,sy + Cell::edge,sx + Cell::size - Cell::edge,sy + Cell::size - Cell::edge,
                0,0,0,255);
      }else{
        if(state == STATE_SELECTED){
          boxRGBA(renderer,sx + Cell::edge,sy + Cell::edge,sx + Cell::size - Cell::edge,sy + Cell::size - Cell::edge,
                  192,192,192,255);
        }else if(state == STATE_PUT){
          boxRGBA(renderer,sx + Cell::edge,sy + Cell::edge,sx + Cell::size - Cell::edge,sy + Cell::size - Cell::edge,
                  255,0,0,255);
        }else{
          boxRGBA(renderer,sx + Cell::edge,sy + Cell::edge,sx + Cell::size - Cell::edge,sy + Cell::size - Cell::edge,
                  255,255,255,255);
        }
      }
    }
  }
}
