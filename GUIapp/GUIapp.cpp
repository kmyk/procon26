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
#include "Keyboard.hpp"
#include "SubWindow.hpp"

GUIapp::GUIapp(){
  SDL_Init(SDL_INIT_EVERYTHING);
  State::create();
  Mouse::create();
  Keyboard::create();
  state = State::instance();
  mouse = Mouse::instance();
  keyboard = Keyboard::instance();
  state->load_input();
  state->parse_input();
  main_window = new MainWindow();
  sub_window = new SubWindow();
}
GUIapp::~GUIapp(){
  SAFE_DELETE(main_window);
  SAFE_DELETE(sub_window);
  Mouse::destroy();
  State::destroy();
  Keyboard::destroy();
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
        mouse->add_button_event(ev.window.windowID,ev.button);
        //if(key == SDLK_ESCAPE){ return false; }
      }
        break;
      case SDL_MOUSEBUTTONUP:
      {
        mouse->add_button_event(ev.window.windowID,ev.button);
        break;
      }
      case SDL_MOUSEMOTION:
      {
        mouse->add_motion_event(ev.window.windowID,ev.motion);
        break;
      }
      case SDL_KEYDOWN:
      {
        //printf("key down\n");
        keyboard->keyon(ev.key.keysym.sym);
        break;
      }
      case SDL_KEYUP:
      {
        keyboard->keyoff(ev.key.keysym.sym);
        break;
      }

    }
  }
  return true;
}

void GUIapp::update(){
  main_window->update();
  sub_window->update();
}
void GUIapp::draw(){
  main_window->draw();
  sub_window->draw();
}

void GUIapp::main_loop(){
  while(polling_event()){
    update();
    draw();
    SDL_Delay((Uint32)timer_wait_mil);
  }
}
