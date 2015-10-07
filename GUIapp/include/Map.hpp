#ifndef __MAP_HPP__
#define __MAP_HPP__

#include <SDL.h>
class Cell;
class Object;
class State;
class Map : public Object{
private:
  Cell** cell;
  State* state;
public:
  static const int colum = 32;
  static const int row = 32;
  Map();
  ~Map();
  void update();
  void draw(SDL_Renderer*);
  void put_stone(int,int);
  bool can_put_stone(int,int);
  bool in_area(int,int);
};

#endif
