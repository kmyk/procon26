#include "common.hpp"

std::istream & operator >> (std::istream & input, board_t & a)  {
    repeat (y,board_size) {
        repeat (x,board_size) {
            char c; input >> c;
            a.a[y][x] = c != '0';
        }
    }
    return input;
}
std::istream & operator >> (std::istream & input, block_t & a)  {
    repeat (y,block_size) {
        repeat (x,block_size) {
            char c; input >> c;
            a.a[y][x] = c != '0';
        }
    }
    return input;
}
std::istream & operator >> (std::istream & input, input_t & a)  {
    input >> a.board;
    // > 石の個数は 1 個以上，256 個以下です。
    int n; input >> n;
    assert (1 <= n and n <= 256);
    a.blocks.resize(n);
    repeat (i,n) input >> a.blocks[i];
    return input;
}


rot_t rot90(rot_t r) {
    return static_cast<rot_t>((r + 1) % 4);
}
bool operator == (point_t const & a, point_t const & b) {
    return a.x == b.x and a.y == b.y;
}
bool operator < (point_t const & a, point_t const & b) {
    return std::make_pair(a.x, a.y) < std::make_pair(b.x, b.y);
}
point_t operator + (point_t const & a, point_t const & b) {
    return (point_t) { a.x + b.x, a.y + b.y };
}
point_t operator - (point_t const & a, point_t const & b) {
    return (point_t) { a.x - b.x, a.y - b.y };
}

std::ostream & operator << (std::ostream & output, flip_t a) { return output << (a == H ? 'H' : 'T'); }
std::ostream & operator << (std::ostream & output, rot_t a)  { return output << (a == R0 ? 0 : a == R90 ? 90 : a == R180 ? 180 : 270); }
std::ostream & operator << (std::ostream & output, placement_t a)  {
    if (a.used) output << a.p.x << " " << a.p.y << " " << a.f << " " << a.r;
    return output;
}
std::ostream & operator << (std::ostream & output, output_t const & a)  {
    for (auto p : a.ps) output << p << std::endl;
    return output;
}
std::ostream & operator << (std::ostream & output, point_t const & a) { return output << "(" << a.x << ", " << a.y << ")"; }
