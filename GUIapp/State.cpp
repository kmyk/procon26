#include "State.hpp"
#include "Utility.hpp"

State::State(int _now_game){
  now_game = _now_game;
  input = new char[1024];
}
State::~State(){
  //SAFE_DELETE(input);
  delete[] input;
}
