#ifndef __CELL_HPP__
#define __CELL_HPP__

class Cell{
public:
  enum Cstate{
      STATE_FILL,
      STATE_EMPTY,
  };
private:
  Cstate state;
  bool is_fill(){ return state == STATE_FILL; }
  bool is_empty(){ return state == STATE_EMPTY; }
  void set_fill(){ state = STATE_FILL; }
  void set_empty(){ state = STATE_EMPTY; }  
};

#endif
