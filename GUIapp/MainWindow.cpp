#include <SDL.h>
#include "Object.hpp"
#include "Mouse.hpp"
#include "Map.hpp"
#include "MainWindow.hpp"
#include "Cell.hpp"
#include "Utility.hpp"

MainWindow::MainWindow(int _w,int _h){
  win_rect.w = Cell::size * 32;
  win_rect.h = Cell::size * 32;
  SDL_CreateWindowAndRenderer(win_rect.w, win_rect.h,0,&window,&renderer);
  map = new Map();
  mouse = Mouse::instance();
}
MainWindow::~MainWindow(){
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SAFE_DELETE(map);
}

void MainWindow::update(){
  if(mouse->exist_button_event()){
    SDL_MouseButtonEvent bevent = mouse->get_button_event();
    if(bevent.type == SDL_MOUSEBUTTONUP){
      int cx = bevent.x / Cell::size;
      int cy = bevent.y / Cell::size;
      if(map->can_put_stone(cx,cy))
        map->put_stone(cx,cy);
    }
  }
}

void MainWindow::draw(){
  SDL_RenderClear(renderer);
  map->draw(renderer);
  SDL_RenderPresent(renderer);
}
