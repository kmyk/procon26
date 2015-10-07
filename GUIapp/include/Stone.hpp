#ifndef __STONE_HPP__
#define __STONE_HPP__

class Stone{
public:
  bool **geometry;  
  static const int row = 8;
  static const int colum = 8;
  Stone();
  ~Stone();
};

#endif
