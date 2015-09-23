#pragma once
#include <vector>
#include <set>
#include <cassert>
#include <cstdint>
#include "common.hpp"

template <typename T, int N>
void copy_cell(T const (& src)[N][N], T (& dst)[N][N]) {
    repeat (y,N) {
        repeat (x,N) {
            dst[y][x] = src[y][x];
        }
    }
}
template <typename T, int N>
void copy_cell_rot90(T const (& src)[N][N], T (& dst)[N][N]) {
    repeat (y,N) {
        repeat (x,N) {
            // 8×8 のマス全体を時計回りに 90 度，180 度，270 度回転させることができる。
            dst[x][N-1-y] = src[y][x];
        }
    }
}
template <typename T, int N>
void copy_cell_flip(T const (& src)[N][N], T (& dst)[N][N]) {
    repeat (y,N) {
        repeat (x,N) {
            // > 裏返した後は，与えられたときの配置と左右に反転しているものとする。
            dst[y][N-x-1] = src[y][x];
        }
    }
}
template <typename T, typename S, int N, typename F>
void copy_cell_map(T const (& src)[N][N], S (& dst)[N][N], F f) {
    repeat (y,N) {
        repeat (x,N) {
            dst[y][x] = f(src[y][x]);
        }
    }
}

template <class F, class G>
void shrink_cell_helper(int n, F pred, G update) {
    while (true) {
        repeat (i,n) {
            if (pred(i)) return; // if something found, exit
        }
        update(); // shrink
    }
}
template <typename T, int N>
void shrink_cell(T const (& a)[N][N], point_t & offset, point_t & size, T target) {
    int & x = offset.x;
    int & y = offset.y;
    int & w = size.x;
    int & h = size.y;
    shrink_cell_helper(h, [&](int dy){ return a[y+dy ][x    ] != target; }, [&](){ x += 1; w -= 1; });
    shrink_cell_helper(w, [&](int dx){ return a[y    ][x+dx ] != target; }, [&](){ y += 1; h -= 1; });
    shrink_cell_helper(h, [&](int dy){ return a[y+dy ][x+w-1] != target; }, [&](){         w -= 1; });
    shrink_cell_helper(w, [&](int dx){ return a[y+h-1][x+dx ] != target; }, [&](){         h -= 1; });
}
template <typename T, int N>
int area_cell(T const (& a)[N][N], T target) {
    int n = 0;
    repeat (y,N) {
        repeat (x,N) {
            if (a[y][x] == target) {
                n += 1;
            }
        }
    }
    return n;
}
template <typename T, int N>
std::vector<point_t> collect_cell(T const (& a)[N][N], point_t const & offset, point_t const & size) {
    std::vector<point_t> result;
    repeat (y,size.y) {
        repeat (x,size.x) {
            if (a[offset.y+y][offset.x+x]) result.push_back({ x, y });
        }
    }
    return result;
}

uint64_t signature_block(bool const (& a)[block_size][block_size], point_t const & offset);

/**
 * @brief 役割: 原点と大きさの概念と置いた石の情報の吸収
 */
class board {
public:
    static constexpr int N = board_size;
private:
    // pimplする？
    int m_cell[N][N];
    point_t m_offset;
    point_t m_size;
    int m_area;
    bool m_is_new;
public:
    board();
    board(board_t const & a);
    void shrink();

public:
    point_t offset() const;
    point_t size() const;
    /**
     * @return
     *   - 0: 何も置かれていない
     *   - 1: 障害物
     *   - 2+n: n番目の石
     */
    int at(point_t p) const;
    int area() const;
    int w() const;
    int h() const;
    /**
     * @attention 破壊的
     */
    void put(point_t p, int n);
    /**
     * @brief どこにも石が置かれていないか
     */
    bool is_new() const;
};

/**
 * @brief 役割: 原点と大きさと回転と反転の概念の吸収
 */
class block {
public:
    static constexpr int N = block_size;
private:
    bool m_cell[2][4][N][N]; // m_cells?
    point_t m_offset[2][4];
    point_t m_size[2][4];
    int m_area;
    bool m_duplicated[2][4]; // true => you should ignore it
    std::vector<point_t> m_stones[2][4];
public:
    block();
    block(block_t const & a);

public:
    point_t offset(flip_t f, rot_t r) const;
    point_t size(flip_t f, rot_t r) const;
    point_t offset(placement_t const & p) const;
    point_t size(placement_t const & p) const;
    int area() const;
    int w(flip_t f, rot_t r) const;
    int h(flip_t f, rot_t r) const;
    int w(placement_t const & p) const;
    int h(placement_t const & p) const;
    point_t local(flip_t f, rot_t r, point_t const & q) const;
    bool at(flip_t f, rot_t r, point_t p) const;
    point_t world(placement_t const & p, point_t const & q) const;
    bool is_duplicated(flip_t f, rot_t r) const;
    std::vector<point_t> const & stones(flip_t f, rot_t r) const;
};
