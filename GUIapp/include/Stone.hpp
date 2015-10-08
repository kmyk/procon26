#ifndef __STONE_HPP__
#define __STONE_HPP__

#include <SDL.h>
class Stone{
private:
  int pX;
  int pY;
public:
  bool **geometry;
  static const int row = 8;
  static const int colum = 8;
  Stone();
  ~Stone();
  void preview(SDL_Renderer*);
  void set_preview_pos(int,int);
};

#endif
