#include "Cell.hpp"
#include "Object.hpp"
#include "Map.hpp"
#include "State.hpp"
#include "Stone.hpp"

Map::Map(){
  state = State::instance();
  cell = new Cell*[row];
  for(int y = 0; y < row; y++){
      cell[y] = new Cell[colum];
      for(int x = 0; x < colum; x++){
        cell[y][x].set_pos(x,y);
        if(state->map_geo[y][x])
          cell[y][x].set_fill();
        else
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

bool Map::in_area(int x,int y){
  return 0 <= x && x < colum && 0 <= y && y < row;
}
static int dx[4] = {1,0,-1,0};
static int dy[4] = {0,1,0,-1};

bool Map::can_put_stone(int x,int y){
  Stone* stone = state->stones[state->now_stone];
  if(stone->get_stone_state() == Stone::STATE_PUT) return false;
  bool exist_connect = false;
  for(int sy = 0; sy < Stone::row; sy++){
    for(int sx = 0; sx < Stone::colum; sx++){
      if(stone->geometry[sy][sx]){
        int nx = x + sx;
        int ny = y + sy;
        if(!in_area(nx,ny)) return false; //Out of area
        if(cell[ny][nx].is_fill()){ //Overlap
          return false;
        }
        /* Exist connection to any other stones */
        for(int d = 0; d < 4; d++){
          if(!in_area(nx + dx[d],ny + dy[d])) continue;
          if(cell[ny + dy[d]][nx + dx[d]].is_fill()){
            exist_connect = true;
            break;
          }
        }
      }
    }
  }
  return exist_connect;
}

int Map::get_score(){
  int empty = 0;
  for(int y = 0; y < row; y++){
    for(int x = 0; x < colum; x++){
      //printf("%c",cell[y][x].is_empty() ? '.' : '#');
      if(!cell[y][x].is_fill()) empty++;
    }
    //printf("\n");
  }
  return empty;
}

void Map::put_stone(int x,int y){
  Stone* stone = state->stones[state->now_stone];
  for(int sy = 0; sy < Stone::row; sy++){
    for(int sx = 0; sx < Stone::colum; sx++){
      int nx = x + sx;
      int ny = y + sy;
      if(stone->geometry[sy][sx])
        cell[ny][nx].set_fill();
    }
  }
  state->stones[state->now_stone]->set_stone_state(Stone::STATE_PUT);
  int score = get_score();
  printf("Fill: %d\tSpace: %d\n",row * colum - score,score);
}

void Map::preview(int x,int y){
  if(x < 0 || y < 0) return;
  for(int y = 0; y < row; y++){
    for(int x = 0; x < colum; x++){
      if(cell[y][x].is_fill_p() || cell[y][x].is_empty_p()) cell[y][x].set_empty();
    }
  }
  Stone* stone = state->stones[state->now_stone];
  for(int sy = 0; sy < Stone::row; sy++){
    for(int sx = 0; sx < Stone::colum; sx++){
      int nx = x + sx;
      int ny = y + sy;
      if(!in_area(nx,ny)) continue;
      if(stone->geometry[sy][sx])
        cell[ny][nx].set_fill_p();
      else
        cell[ny][nx].set_empty_p();
    }
  }
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
