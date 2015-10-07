#ifndef __STATE_HPP__
#define __STATE_HPP__

#include <vector>
#include "Stone.hpp"

/* ### Singleton class ### */
class State{
private:
  State(int);
  ~State();
  State(const State&){;}
  static State* mInstance;
  std::vector<Stone*>stones;
  int stone_num;
  bool **map_geo;
public:
  char* input;
  char* output;
  int now_game;
  int now_stone;
  static State* instance();
  static void create();
  static void destroy();
  void parse_input();
};

#endif
