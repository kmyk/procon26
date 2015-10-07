#include "Object.hp"
#include "Map.hpp"
#include "MainWindow.hpp"

MainWindow::MainWindow(int _w,int _h){
  win_rect.w = _w;
  win_rect.h = _h;
  SDL_CreateWindowAndRenderer(win_rect.w, win_rect.h,0,&window,&renderer);
  map = new Map();
}
MainWindow::~MainWindow(){
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}

void MainWindow::draw(){
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);
  map->draw();
}
