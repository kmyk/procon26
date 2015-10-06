#ifndef __MAIN_WINDOW_HPP__
#define __MAIN_WINDOW_HPP__

#include <SDL.h>

class MainWindow{
private:
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Rect win_rect;
public:
  MainWindow(int,int);
  ~MainWindow();
};

#endif
