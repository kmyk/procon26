#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "Object.hpp"
#include "Map.hpp"
#include "State.hpp"
#include "Utility.hpp"

State* State::mInstance = 0;
State* State::instance(){
  return mInstance;
}
void State::create(){
  EXCEPT_NORMAL_BEGIN;
  if(mInstance) throw "State::create() cannnot create instance twice";
  mInstance = new State(1);
  EXCEPT_NORMAL_END;
}
void State::destroy(){
  SAFE_DELETE(mInstance);
}

State::State(int _now_game){
  now_game = _now_game;
  now_stone = 0;
  stone_num = 0;
  input_size = 0;
  input = new char[1024];
  map_geo = new bool*[Map::row];
  for(int y = 0; y < Map::row; y++){
      map_geo[y] = new bool[Map::colum];
      for(int x = 0; x < Map::colum; x++){
        map_geo[y][x] = false;
      }
  }
}
State::~State(){
  delete[] input;
  for(int y = 0; y < Map::row; y++){
      delete [] map_geo[y];
  }
  for(int s = 0; s < stone_num; s++){
    delete stones[s];
  }
  stones.clear();
  delete[] map_geo;

}
void State::load_input(){
  char filepath[256];
  sprintf(filepath,"input/quest%d.txt",now_game);
  EXCEPT_NORMAL_BEGIN;
  FILE* fp;
  if((fp = fopen(filepath,"rb")) == NULL) throw "input file open error";
  fseek(fp,0,SEEK_END);
  input_size = ftell(fp);
  fseek(fp,0,SEEK_SET);
  input = new char[input_size];
  fread(input,sizeof(char),input_size,fp);
  fclose(fp);
  EXCEPT_NORMAL_END;
}
static int nextline = 2;
void State::parse_input(){
  /* >>>> Super f*cking DIRTY code. I'm too lazy to build parser class;-) <<<< */
  for(int i = 0; i < input_size; i++) printf("%c",input[i]);
  int ptr = 0;
  for(int y = 0; y < Map::row; y++){
    for(int x = 0; x < Map::colum; x++){
      map_geo[y][x] = (input[ptr] == '1' ? true : false);
      ptr++;
    }
    ptr += nextline;
  }
  ptr += nextline; //nextline
  stone_num = 0;
  int base = 10;
  while('0' <= input[ptr] && input[ptr] <= '9'){
    printf("%c",input[ptr]);
    stone_num *= base;
    stone_num += input[ptr] - '0';
    ptr++;
  }
  printf("\nstone_num:%d\n",stone_num);
  ptr += nextline; //nextline
  for(int s = 0; s < stone_num; s++){
    stones.push_back(new Stone());
    for(int y = 0; y < Stone::row; y++){
      for(int x = 0; x < Stone::colum; x++){
        if(input[ptr] == '1')
          stones[stones.size() - 1]->geometry[y][x] = true;
        ptr++;
      }
      ptr += nextline;
    }
    ptr += nextline;
  }
}
