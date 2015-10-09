#ifndef __CELL_HPP__
#define __CELL_HPP__

#include <SDL.h>

class Cell{
public:
  enum Cstate{
      STATE_FILL,
      STATE_EMPTY,
      STATE_FILL_P,
      STATE_EMPTY_P,
  };
private:
  Cstate state;
  int x;
  int y;
public:
  Cell(){;}
  Cell(int,int);
  static const int size =  15;
  static const int edge = 2;
  bool is_fill(){ return state == STATE_FILL; }
  bool is_empty(){ return state == STATE_EMPTY; }
  bool is_fill_p(){ return state == STATE_FILL_P; }
  bool is_empty_p(){ return state == STATE_EMPTY_P; }
  void set_fill(){ state = STATE_FILL; }
  void set_empty(){ state = STATE_EMPTY; }
  void set_fill_p(){ if(state == STATE_EMPTY) state = STATE_FILL_P; }
  void set_empty_p(){ if(state == STATE_EMPTY) state = STATE_EMPTY_P; }

  void set_pos(int,int);
  void draw(SDL_Renderer*);
};

#endif
