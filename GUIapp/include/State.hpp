#ifndef __STATE_HPP__
#define __STATE_HPP__

class State{
public:
  char* input;
  char* output;
  int now_game;
  State(int);
  ~State();
};

#endif
