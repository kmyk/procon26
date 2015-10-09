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

bool Mouse::exist_button_event(Uint32 key){
  return !button_events[key].empty();
}

SDL_MouseButtonEvent Mouse::get_button_event(Uint32 key){
  SDL_MouseButtonEvent topevent = button_events[key].front();
  button_events[key].pop_front();
  return topevent;
}

void Mouse::add_button_event(Uint32 key,SDL_MouseButtonEvent _event){
  button_events[key].push_back(_event);
}

bool Mouse::exist_motion_event(Uint32 key){
  return !motion_events[key].empty();
}

SDL_MouseMotionEvent Mouse::get_motion_event(Uint32 key){
  SDL_MouseMotionEvent topevent = motion_events[key].front();
  motion_events[key].pop_front();
  return topevent;
}

void Mouse::add_motion_event(Uint32 key,SDL_MouseMotionEvent _event){
  motion_events[key].push_back(_event);
}
