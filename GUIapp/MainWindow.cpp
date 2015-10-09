#include <SDL.h>
#include "Object.hpp"
#include "Mouse.hpp"
#include "Map.hpp"
#include "MainWindow.hpp"
#include "Cell.hpp"
#include "Utility.hpp"
#include "State.hpp"
#include "Stone.hpp"

MainWindow::MainWindow(){
  win_rect.w = Cell::size * (Map::colum + Map::edge * 2);
  win_rect.h = Cell::size * (Map::row + Map::edge * 2);
  SDL_CreateWindowAndRenderer(win_rect.w, win_rect.h,0,&window,&renderer);
  map = new Map();
  mouse = Mouse::instance();
  state = State::instance();
  char filepath[256];
  sprintf(filepath,"output/quest%d.out",state->now_game);
  output_fp = fopen(filepath,"w");

}
MainWindow::~MainWindow(){
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SAFE_DELETE(map);
  fclose(output_fp);
}

void MainWindow::output_res(){
  Stone* stone = state->stones[state->now_stone];
  char buf[256];
  sprintf(buf,"%d %d %c %d\n",stone->x,stone->y,(stone->flip ? 'T' : 'H'),stone->rotate);
  fwrite(buf,sizeof(char),strlen(buf),output_fp);
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
      if(map->can_put_stone(cx,cy)){
        map->put_stone(cx,cy);
        output_res();
      }
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
  map->update();
}

void MainWindow::draw(){
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);
  map->draw(renderer);
  SDL_RenderPresent(renderer);
}
