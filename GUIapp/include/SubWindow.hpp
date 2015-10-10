#ifndef __SUB_WINDOW_HPP__
#define __SUB_WINDOW_HPP__

#include <SDL.h>

class Mouse;
class State;
class SubWindow{
private:
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Rect win_rect;
  Mouse* mouse;
  State* state;
public:
  static const int interval = 1;
  static const int row = 5;
  static const int colum = 5;
  SubWindow();
  ~SubWindow();
  void init_preview();
  void draw();
  void update();
};

#endif
