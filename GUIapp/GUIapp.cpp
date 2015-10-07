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
  state = new State(1);
  main_window = new MainWindow(500,500);
  load_input();
  Mouse::create();
  mouse = Mouse::instance();
}
GUIapp::~GUIapp(){
  SAFE_DELETE(main_window);
  SAFE_DELETE(state);
  Mouse::destroy();
}

void GUIapp::load_input(){
  char filepath[256];
  sprintf(filepath,"input/quest%d.txt",state->now_game);
  EXCEPT_NORMAL_BEGIN;
  FILE* fp;
  if((fp = fopen(filepath,"r")) == NULL) throw "input file open error";
  fread(state->input,sizeof(char),1024,fp);
  fclose(fp);
  EXCEPT_NORMAL_END;
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
