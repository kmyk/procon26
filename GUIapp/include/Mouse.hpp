#ifndef __MOUSE_HPP__
#define __MOUSE_HPP__

#include <deque>
#include <map>
#include <SDL.h>

/* ### Singleton Class ### */
class Mouse{
private:
  Mouse();
  ~Mouse();
  std::map<Uint32, std::deque<SDL_MouseButtonEvent> > button_events;
  std::map<Uint32, std::deque<SDL_MouseMotionEvent> > motion_events;
public:
  static Mouse* mInstance;
  static Mouse* instance();
  static void create();
  static void destroy();
  SDL_MouseButtonEvent get_button_event(Uint32);
  void add_button_event(Uint32,SDL_MouseButtonEvent);
  bool exist_button_event(Uint32);
  SDL_MouseMotionEvent get_motion_event(Uint32);
  void add_motion_event(Uint32,SDL_MouseMotionEvent);
  bool exist_motion_event(Uint32);

};

#endif
