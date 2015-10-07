#include "Cell.hpp"
#include "Object.hpp"
#include "Map.hpp"
#include "State.hpp"

Map::Map(){
  cell = new Cell*[row];
  for(int y = 0; y < row; y++){
      cell[y] = new Cell[colum];
      for(int x = 0; x < colum; x++){
        cell[y][x].set_pos(x,y);
        cell[y][x].set_empty();
      }
  }
}

Map::~Map(){
  for(int y = 0; y < row; y++){
      delete [] cell[y];
  }
  delete[] cell;
}

void Map::update(){

}

void Map::draw(SDL_Renderer* renderer){
  for(int y = 0; y < row; y++){
    for(int x = 0; x < colum; x++){
        cell[y][x].draw(renderer);
    }
  }
}
