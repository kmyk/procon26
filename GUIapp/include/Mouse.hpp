#ifndef __MOUSE_HPP__
#define __MOUSE_HPP__

#include <deque>
#include <SDL.h>

/* ### Singleton Class ### */
class Mouse{
private:
  Mouse();
  ~Mouse();
  std::deque<SDL_MouseButtonEvent> button_events;
public:
  static Mouse* mInstance;
  static Mouse* instance();
  static void create();
  static void destroy();
  SDL_MouseButtonEvent get_button_event();
  void add_button_event(SDL_MouseButtonEvent);
  bool exist_button_event();
};

#endif
