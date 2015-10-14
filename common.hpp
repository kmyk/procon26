/**
 * @file common.hpp
 * @author Kimiyuki Onaka
 * @brief 極基本的な型/定数/関数とその入出力
 */
#pragma once
#include <iostream>
#include <vector>
#include <cassert>
#define repeat_from(i,m,n) for (int i = (m); (i) < (n); ++(i))
#define repeat(i,n) repeat_from(i,0,n)
#define repeat_from_reverse(i,m,n) for (int i = (n)-1; (i) >= (m); --(i))
#define repeat_reverse(i,n) repeat_from_reverse(i,0,n)
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
/**
 * @brief 問題が規定する形式で入力
 */
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

flip_t flip(flip_t f);
rot_t rot90(rot_t r);
bool operator == (point_t const & a, point_t const & b);
bool operator < (point_t const & a, point_t const & b);
/**
 * @attention 速度のためinline化 まあまあ効果がある
 */
inline point_t operator + (point_t const & a, point_t const & b) {
     return (point_t) { a.x + b.x, a.y + b.y };
}
point_t operator - (point_t const & a, point_t const & b);

std::ostream & operator << (std::ostream & output, flip_t a);
std::ostream & operator << (std::ostream & output, rot_t a) ;
std::ostream & operator << (std::ostream & output, placement_t a) ;
/**
 * @brief 問題が規定する形式で出力
 */
std::ostream & operator << (std::ostream & output, output_t const & a) ;
std::ostream & operator << (std::ostream & output, point_t const & a);

const int dy[5] = { 1, -1, 0, 0, 0 };
const int dx[5] = { 0, 0, 1, -1, 0 };
const point_t dp[5] = { { 0, 1 }, { 0, -1 }, { 1, 0 }, { -1, 0 }, { 0, 0 } };
bool is_on(point_t const & p, point_t const & size);
bool is_on_board(point_t const & p);
/**
 * @brief pointwise min
 */
point_t pwmin(point_t const & a, point_t const & b);
point_t pwmax(point_t const & a, point_t const & b);
int cross(point_t const & a, point_t const & b);
int   dot(point_t const & a, point_t const & b);
