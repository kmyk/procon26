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
public:
  std::vector<Stone*>stones;
  char* input;
  int input_size;
  char* output;
  int now_game;
  int now_stone;
  int stone_num;
  bool **map_geo;
  static State* instance();
  static void create();
  static void destroy();
  void parse_input();
  void load_input();
};

#endif
