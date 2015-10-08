#include <SDL.h>
#include <cstdio>
#include "Object.hpp"
#include "Mouse.hpp"
#include "SubWindow.hpp"
#include "State.hpp"
#include "Utility.hpp"
#include "Cell.hpp"

SubWindow::SubWindow(){
  win_rect.w = (Stone::colum + interval) * Cell::size * colum;
  win_rect.h = (Stone::row + interval) * Cell::size * row;
  SDL_CreateWindowAndRenderer(win_rect.w, win_rect.h,0,&window,&renderer);
  mouse = Mouse::instance();
  state = State::instance();
  init_preview();
}
SubWindow::~SubWindow(){
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}

void SubWindow::init_preview(){
  printf("stone_num: %d,%d\n",state->stone_num,state->stones.size());
  for(int s = 0; s < state->stone_num; s++){
    int x = s % colum;
    int y = s / colum;
    int px = (Stone::colum + interval) * x * Cell::size;
    int py = (Stone::row + interval) * y * Cell::size;
    state->stones[s]->set_preview_pos(px,py);
  }
}
void SubWindow::update(){
  if(mouse->exist_button_event()){
    SDL_MouseButtonEvent bevent = mouse->get_button_event();
    if(bevent.type == SDL_MOUSEBUTTONUP){
    }
  }
}

void SubWindow::draw(){
  SDL_RenderClear(renderer);
  for(int s = 0; s < state->stone_num; s++){
    state->stones[s]->preview(renderer);
  }
  SDL_RenderPresent(renderer);
}
