#include "Cell.hpp"
#include "Object.hpp"
#include "Map.hpp"
#include "State.hpp"
#include "Stone.hpp"
#include "Keyboard.hpp"

Map::Map(){
  state = State::instance();
  keyboard = Keyboard::instance();
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
        int nx = x + sx - edge;
        int ny = y + sy - edge;
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
      int nx = x + sx - edge;
      int ny = y + sy - edge;
      if(stone->geometry[sy][sx]){
        cell[ny][nx].set_fill();
      }
    }
  }
  state->stones[state->now_stone]->set_put_pos(x - edge,y - edge);
  state->stones[state->now_stone]->set_stone_state(Stone::STATE_PUT);
  int score = get_score();
  printf("Fill: %d\tSpace: %d\n",row * colum - score,score);
}

void Map::preview(int _x,int _y){
  if(_x < 0 || _y < 0) return;
  for(int y = 0; y < row; y++){
    for(int x = 0; x < colum; x++){
      if(cell[y][x].is_fill_p() || cell[y][x].is_empty_p()) cell[y][x].set_empty();
    }
  }
  Stone* stone = state->stones[state->now_stone];
  for(int sy = 0; sy < Stone::row; sy++){
    for(int sx = 0; sx < Stone::colum; sx++){
      int nx = _x + sx - edge;
      int ny = _y + sy - edge;
      if(!in_area(nx,ny)) continue;
      if(stone->geometry[sy][sx])
        cell[ny][nx].set_fill_p();
      else
        cell[ny][nx].set_empty_p();
    }
  }
}
void Map::update(){
  if(keyboard->is_keyon(SDLK_SPACE)){
    keyboard->keytoggle(SDLK_SPACE);
    Stone* stone = state->stones[state->now_stone];
    stone->stone_rotate90();
  }
  if(keyboard->is_keyon(SDLK_TAB)){
    keyboard->keytoggle(SDLK_TAB);    
    Stone* stone = state->stones[state->now_stone];
    stone->stone_flip();
  }

}

void Map::draw(SDL_Renderer* renderer){
  for(int y = 0; y < row; y++){
    for(int x = 0; x < colum; x++){
        cell[y][x].draw(renderer);
    }
  }
}
