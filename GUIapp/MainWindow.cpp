#include <SDL.h>
#include "Object.hpp"
#include "Mouse.hpp"
#include "Map.hpp"
#include "MainWindow.hpp"
#include "Cell.hpp"
#include "Utility.hpp"

MainWindow::MainWindow(){
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
  Uint32 win_id = SDL_GetWindowID(window);
  static int mx = -1,my = -1;
  map->preview(mx,my);
  if(mouse->exist_button_event(win_id)){
    SDL_MouseButtonEvent bevent = mouse->get_button_event(win_id);
    if(bevent.type == SDL_MOUSEBUTTONUP){
      int cx = bevent.x / Cell::size;
      int cy = bevent.y / Cell::size;
      if(map->can_put_stone(cx,cy))
        map->put_stone(cx,cy);
    }
  }
  if(mouse->exist_motion_event(win_id)){
    SDL_MouseMotionEvent mevent = mouse->get_motion_event(win_id);
    if(mevent.type == SDL_MOUSEMOTION){
      int cx = mevent.x / Cell::size;
      int cy = mevent.y / Cell::size;
      mx = cx;
      my = cy;
    }
  }

}

void MainWindow::draw(){
  SDL_RenderClear(renderer);
  map->draw(renderer);
  SDL_RenderPresent(renderer);
}
