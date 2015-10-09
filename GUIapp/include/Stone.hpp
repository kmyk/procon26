#ifndef __STONE_HPP__
#define __STONE_HPP__

#include <SDL.h>
class Stone{
public:
  enum SState{
    STATE_SELECTED,
    STATE_PUT,
    STATE_NONE,
  };
private:
  int pX;
  int pY;
  SState state;
public:
  bool **geometry;
  static const int row = 8;
  static const int colum = 8;
  int x;
  int y;
  bool flip;
  int rotate;
  Stone();
  ~Stone();
  void preview(SDL_Renderer*);
  void set_preview_pos(int,int);
  void set_put_pos(int,int);
  void set_stone_state(SState);
  void stone_flip();
  void stone_rotate90();
  SState get_stone_state();
};

#endif
