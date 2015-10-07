#include "Object.hpp"
#include "Map.hpp"
#include "MainWindow.hpp"
#include "Cell.hpp"

MainWindow::MainWindow(int _w,int _h){
  win_rect.w = Cell::size * 32;
  win_rect.h = Cell::size * 32;
  SDL_CreateWindowAndRenderer(win_rect.w, win_rect.h,0,&window,&renderer);
  map = new Map();
}
MainWindow::~MainWindow(){
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}

void MainWindow::update(){
}

void MainWindow::draw(){
  SDL_RenderClear(renderer);
  map->draw(renderer);
  SDL_RenderPresent(renderer);
}
