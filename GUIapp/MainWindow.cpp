#include "MainWindow.hpp"

MainWindow::MainWindow(int _w,int _h){
  win_rect.w = _w;
  win_rect.h = _h;  
  SDL_CreateWindowAndRenderer(win_rect.w, win_rect.h,0,&window,&renderer);
}
MainWindow::~MainWindow(){
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}
