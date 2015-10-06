#include "Mouse.hpp"
#include "Utility.hpp"

Mouse* Mouse::mInstance = 0;
Mouse::Mouse(){}
Mouse::~Mouse(){}
Mouse* Mouse::instance(){
  return mInstance;
}

void Mouse::create(){
  EXCEPT_NORMAL_BEGIN;
  if(mInstance) throw "Mouse cannot create instance twice";
  mInstance = new Mouse();
  EXCEPT_NORMAL_END;
}

void Mouse::destroy(){
  SAFE_DELETE(mInstance);
}

bool Mouse::exist_button_event(){
  return !button_events.empty();
}

SDL_MouseButtonEvent Mouse::get_button_event(){
  SDL_MouseButtonEvent topevent = button_events.front();
  button_events.pop_front();
  return topevent;
}

void Mouse::add_button_event(SDL_MouseButtonEvent _event){
  button_events.push_back(_event);
}
