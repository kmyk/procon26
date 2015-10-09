#ifndef __MAP_HPP__
#define __MAP_HPP__

#include <SDL.h>
class Cell;
class Object;
class State;
class Keyboard;

class Map : public Object{
private:
  Cell** cell;
  State* state;
  Keyboard* keyboard;
public:
  static const int colum = 32;
  static const int row = 32;
  static const int edge = 7;
  Map();
  ~Map();
  void update();
  void draw(SDL_Renderer*);
  void put_stone(int,int);
  bool can_put_stone(int,int);
  bool in_area(int,int);
  int get_score();
  void preview(int,int);
};

#endif
