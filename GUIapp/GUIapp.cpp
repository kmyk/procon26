#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <SDL.h>

#include "State.hpp"
#include "MainWindow.hpp"
#include "GUIapp.hpp"
#include "Utility.hpp"
#include "Mouse.hpp"

GUIapp::GUIapp(){
  SDL_Init(SDL_INIT_EVERYTHING);
  State::create();
  Mouse::create();
  state = State::instance();
  mouse = Mouse::instance();
  state->load_input();
  state->parse_input();
  main_window = new MainWindow(500,500);
}
GUIapp::~GUIapp(){
  SAFE_DELETE(main_window);
  Mouse::destroy();
  State::destroy();
}

bool GUIapp::polling_event(void){
  SDL_Event ev;
  while ( SDL_PollEvent(&ev) ){
    switch(ev.type){
      case SDL_QUIT:
        return false;
        break;
      case SDL_MOUSEBUTTONDOWN:
      {
        mouse->add_button_event(ev.button);
        //if(key == SDLK_ESCAPE){ return false; }
      }
        break;
      case SDL_MOUSEBUTTONUP:
      {
        mouse->add_button_event(ev.button);
        break;
      }
    }
  }
  return true;
}

void GUIapp::update(){
  main_window->update();
}
void GUIapp::draw(){
  main_window->draw();
}

void GUIapp::main_loop(){
  while(polling_event()){
    update();
    draw();
    SDL_Delay((Uint32)timer_wait_mil);
  }
}
