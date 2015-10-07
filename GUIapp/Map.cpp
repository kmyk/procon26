#include "Cell.hpp"
#include "Object.hpp"
#include "Map.hpp"

Map::Map(){
  cell = new Cell[colum * row];
}

Map::~Map(){
  delete[] cell;
}

void Map::update(){

}

void Map::draw(){

}
