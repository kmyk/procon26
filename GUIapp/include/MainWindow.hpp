#ifndef __MAIN_WINDOW_HPP__
#define __MAIN_WINDOW_HPP__

#include <SDL.h>
class Map;
class Mouse;
class State;

class MainWindow{
private:
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Rect win_rect;
  Map* map;
  Mouse* mouse;
  FILE* output_fp;
  State* state;
public:
  MainWindow();
  ~MainWindow();
  void draw();
  void update();
  void output_res();
};

#endif
