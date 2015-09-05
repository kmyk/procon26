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
    // 石を置けるか
    bool a[board_size][board_size]; // '0' => true
};
struct block_t {
    bool a[block_size][block_size]; // '1' => true
};
struct input_t {
    board_t board;
    std::vector<block_t> blocks;
};

inline std::istream & operator >> (std::istream & input, board_t & a)  {
    repeat (y,board_size) {
        repeat (x,board_size) {
            char c; input >> c;
            a.a[y][x] = c == '0';
        }
    }
    return input;
}
inline std::istream & operator >> (std::istream & input, block_t & a)  {
    repeat (y,board_size) {
        repeat (x,board_size) {
            char c; input >> c;
            a.a[y][x] = c == '1';
        }
    }
    return input;
}
inline std::istream & operator >> (std::istream & input, input_t & a)  {
    input >> a.board;
    // > 石の個数は 1 個以上，256 個以下です。
    int n; input >> n;
    assert (1 <= n and n <= 256);
    a.blocks.resize(n);
    repeat (i,n) input >> a.blocks[i];
    return input;
}

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
inline rot_t rot90(rot_t r) {
    return static_cast<rot_t>((r + 1) % 4);
}

inline std::ostream & operator << (std::ostream & output, flip_t a) { return output << (a == H ? 'H' : 'T'); }
inline std::ostream & operator << (std::ostream & output, rot_t a)  { return output << (a == R0 ? 0 : a == R90 ? 90 : a == R180 ? 180 : 270); }
inline std::ostream & operator << (std::ostream & output, placement_t a)  {
    if (a.used) output << a.p.x << " " << a.p.y << " " << a.f << " " << a.r;
    return output;
}
inline std::ostream & operator << (std::ostream & output, output_t const & a)  {
    for (auto p : a.ps) output << p << std::endl;
    return output;
}
inline std::ostream & operator << (std::ostream & output, point_t const & a) { return output << "(" << a.x << ", " << a.y << ")"; }
