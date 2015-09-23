#pragma once
#include <iostream>
#include <vector>
#include <cassert>
#define repeat_from(i,m,n) for (int i = (m); (i) < (n); ++(i))
#define repeat(i,n) repeat_from(i,0,n)
typedef long long ll;

constexpr int board_size = 32;
constexpr int block_size = 8;

constexpr int max_block_number = 256;
constexpr int max_block_area = 16;

struct board_t {
    // 障害物があるか
    bool a[board_size][board_size];
};
struct block_t {
    bool a[block_size][block_size];
};
struct input_t {
    board_t board;
    std::vector<block_t> blocks;
};

std::istream & operator >> (std::istream & input, board_t & a) ;
std::istream & operator >> (std::istream & input, block_t & a) ;
std::istream & operator >> (std::istream & input, input_t & a) ;

enum flip_t { H, T };
enum rot_t { R0, R90, R180, R270 };
struct point_t { int x, y; };
struct placement_t {
    bool used;
    point_t p;
    flip_t f;
    rot_t r;
};
struct output_t {
    std::vector<placement_t> ps;
};

rot_t rot90(rot_t r);
bool operator == (point_t const & a, point_t const & b);
bool operator < (point_t const & a, point_t const & b);
point_t operator + (point_t const & a, point_t const & b);
point_t operator - (point_t const & a, point_t const & b);

std::ostream & operator << (std::ostream & output, flip_t a);
std::ostream & operator << (std::ostream & output, rot_t a) ;
std::ostream & operator << (std::ostream & output, placement_t a) ;
std::ostream & operator << (std::ostream & output, output_t const & a) ;
std::ostream & operator << (std::ostream & output, point_t const & a);
