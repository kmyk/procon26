#ifndef __GUIAPP_HPP__
#define __GUIAPP_HPP__

class MainWindow;
class SubWindow;
class State;
class Mouse;

class GUIapp{
private:
  State* state;
  MainWindow* main_window;
  SubWindow* sub_window;
  Mouse* mouse;
  const int fps = 10;
  const int timer_wait_mil = 1 / fps * 1000;
public:
  GUIapp();
  ~GUIapp();
  void main_loop();
  bool polling_event();
  void update();
  void draw();
};

#endif
