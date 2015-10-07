#ifndef __CELL_HPP__
#define __CELL_HPP__

#include <SDL.h>

class Cell{
public:
  enum Cstate{
      STATE_FILL,
      STATE_EMPTY,
  };
private:
  Cstate state;
  int x;
  int y;
public:
  Cell(){;}
  Cell(int,int);
  static const int size =  20;
  static const int edge = 3;
  bool is_fill(){ return state == STATE_FILL; }
  bool is_empty(){ return state == STATE_EMPTY; }
  void set_fill(){ state = STATE_FILL; }
  void set_empty(){ state = STATE_EMPTY; }
  void set_pos(int,int);
  void draw(SDL_Renderer*);
};

#endif
