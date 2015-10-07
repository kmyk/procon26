#include "Stone.hpp"

Stone::Stone(){
  geometry = new bool*[row];
  for(int y = 0; y < row; y++){
      geometry[y] = new bool[colum];
      for(int x = 0; x < colum; x++){
        geometry[y][x] = false;
      }
  }
}

Stone::~Stone(){
  for(int y = 0; y < row; y++){
      delete [] geometry[y];
  }
  delete[] geometry;

}
