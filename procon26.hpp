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

uint64_t signature_block(bool const (& a)[block_size][block_size], point_t const & offset) {
    static_assert (block_size == 8, "");
    uint64_t z = 0; // 8*8 == 64
    repeat_from (y, offset.y, 8) {
        uint64_t w = 0;
        repeat_from (x, offset.x, 8) {
            w = w << 1;
            if (a[y][x]) w = w | 1;
        }
        w = w << offset.x;
        z = (z << 8) | w;
    }
    z = z << (8 * offset.y);
    return z;
}

/**
 * @brief 役割: 原点と大きさの概念と置いた石の情報の吸収
 */
class board {
public:
    static constexpr int N = board_size;
private:
    int m_cell[N][N];
    point_t m_offset;
    point_t m_size;
    int m_area;
public:
    board() = default;
    board(board_t const & a) {
        m_offset = { 0, 0 };
        m_size = { N, N };
        copy_cell_map(a.a, m_cell, [](bool x) -> int { return x; });
        shrink();
    }
    void shrink() {
        m_area = area_cell(m_cell, 0);
        shrink_cell(m_cell, m_offset, m_size, 1);
    }

public:
    point_t offset() const { return m_offset; }
    point_t size() const { return m_size; }
    /**
     * @return
     *   - 0: 何も置かれていない
     *   - 1: 障害物
     *   - 2+n: n番目の石
     */
    int at(point_t p) const {
        point_t q = offset();
        return m_cell[p.y+q.y][p.x+q.x];
    }
    int area() const { return m_area; }
    int w() const { return m_size.x; }
    int h() const { return m_size.y; }
    /**
     * @attention 破壊的
     */
    void put(point_t p, int n) {
        point_t q = offset();
        int & it = m_cell[p.y+q.y][p.x+q.x];
        assert (it == 0);
        it = 2+n;
    }
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
    block() = default;
    block(block_t const & a) {
        for (flip_t f : { H, T }) {
            for (rot_t r : { R0, R90, R180, R270 }) {
                m_offset[f][r] = { 0, 0 };
                m_size[f][r] = { N, N };
            }
        }
        // > 石は 1 個以上かつ，16 個以下のブロックにより構成され，
        m_area = area_cell(a.a, true);
        assert (1 <= m_area and m_area <= 16);
        // > 石操作の順番として，はじめに石の裏表を決め，そのあとに回転させる。
        copy_cell(a.a, m_cell[H][R0]);
        copy_cell_flip(m_cell[H][R0], m_cell[T][R0]);
        for (flip_t f : { H, T }) {
            for (rot_t r : { R0, R90, R180 }) {
                copy_cell_rot90(m_cell[f][r], m_cell[f][rot90(r)]);
            }
        }
        for (flip_t f : { H, T }) {
            for (rot_t r : { R0, R90, R180, R270 }) {
                shrink_cell(m_cell[f][r], m_offset[f][r], m_size[f][r], false);
                m_stones[f][r] = collect_cell(m_cell[f][r], m_offset[f][r], m_size[f][r]);
            }
        }
        std::set<uint64_t> sigs;
        for (flip_t f : { H, T }) {
            for (rot_t r : { R0, R90, R180, R270 }) {
                uint64_t sig = signature_block(m_cell[f][r], m_offset[f][r]);
                m_duplicated[f][r] = sigs.count(sig);
                sigs.insert(sig);
            }
        }
    }

public:
    bool const (& cell(flip_t f, rot_t r) const) [N][N] { return m_cell[f][r]; }
    point_t offset(flip_t f, rot_t r) const { return m_offset[f][r]; }
    point_t size(flip_t f, rot_t r) const { return m_size[f][r]; }
    point_t offset(placement_t const & p) const { assert (p.used); return offset(p.f,p.r); }
    point_t size(placement_t const & p) const { assert (p.used); return size(p.f,p.r); }
    int area() const { return m_area; }
    int w(flip_t f, rot_t r) const { return size(f,r).x; }
    int h(flip_t f, rot_t r) const { return size(f,r).y; }
    int w(placement_t const & p) const { return w(p.f,p.r); }
    int h(placement_t const & p) const { return h(p.f,p.r); }
    point_t local(flip_t f, rot_t r, point_t const & q) const {
        int ay = offset(f,r).y + q.y;
        int ax = offset(f,r).x + q.x;
        return { ax, ay };
    }
    bool at(flip_t f, rot_t r, point_t p) const {
        p = local(f,r,p);
        return m_cell[f][r][p.y][p.x];
    }
    point_t world(placement_t const & p, point_t const & q) const {
        assert (p.used);
        int ay = p.p.y + offset(p.f,p.r).y + q.y;
        int ax = p.p.x + offset(p.f,p.r).x + q.x;
        return { ax, ay };
    }
    bool is_duplicated(flip_t f, rot_t r) const { return m_duplicated[f][r]; }
    std::vector<point_t> const & stones(flip_t f, rot_t r) const { return m_stones[f][r]; }
};
