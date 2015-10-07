#ifndef __MAP_HPP__
#define __MAP_HPP__

class Cell;
class Object;
class Map : public Object{
private:
  const int colum = 32;
  const int row = 32;
  Cell* cell;
public:
  Map();
  ~Map();
  void update();
  void draw();
};

#endif
