#include <SDL2_gfxPrimitives.h>
#include <cstdio>
#include "Stone.hpp"
#include "Cell.hpp"
#include "Object.hpp"
#include "Map.hpp"

Stone::Stone(){
  pX = 0;
  pY = 0;
  state = STATE_NONE;
  flip = false;
  rotate = 0;
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
void Stone::set_put_pos(int _x,int _y){
  x = _x;
  y =_y;
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
void Stone::stone_flip(){
  //printf("flip\n");
  flip = !flip;
  bool** tmp = new bool*[row];
  for(int y = 0; y < row; y++){
      tmp[y] = new bool[colum];
      for(int x = 0; x < colum; x++){
        tmp[y][x] = geometry[y][x];
        geometry[y][x] = false;
      }
  }

  for(int y = 0; y < row; y++){
    for(int x = 0; x < colum; x++){
        int nx = colum - 1 - x;
        int ny = y;
        geometry[ny][nx] = tmp[y][x];
        //printf("(%d,%d) => (%d,%d)\n",y,x,ny,nx);
    }
  }

  for(int y = 0; y < row; y++){
      delete [] tmp[y];
  }
  delete[] tmp;

}
void Stone::stone_rotate90(){
  rotate += 90;
  rotate %= 360;
  bool** tmp = new bool*[row];
  for(int y = 0; y < row; y++){
      tmp[y] = new bool[colum];
      for(int x = 0; x < colum; x++){
        tmp[y][x] = geometry[y][x];
        geometry[y][x] = false;
      }
  }

  for(int y = 0; y < row; y++){
    for(int x = 0; x < colum; x++){
        int nx = colum - 1 - y;
        int ny = x;
        geometry[ny][nx] = tmp[y][x];
    }
  }

  for(int y = 0; y < row; y++){
      delete [] tmp[y];
  }
  delete[] tmp;

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
