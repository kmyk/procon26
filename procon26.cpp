#include "procon26.hpp"

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
            point_t p = offset + (point_t) { x, y };
            if (a[p.y][p.x]) result.push_back(p);
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


board::board() = default;
board::board(board_t const & a) {
    m_offset = { 0, 0 };
    m_size = { N, N };
    copy_cell_map(a.a, m_cell, [](bool x) -> int { return x; });
    update();
    shrink();
}
void board::update() {
    m_area = area_cell(m_cell, 0);
    int obstacles = area_cell(m_cell, 1);
    m_is_new = (m_area + obstacles == N * N) ? 1 : 0;
}
void board::shrink() {
    shrink_cell(m_cell, m_offset, m_size, 1);
}

point_t board::offset() const { return m_offset; }
point_t board::size() const { return m_size; }
int board::at_local(point_t p) const {
    point_t q = offset();
    return m_cell[p.y+q.y][p.x+q.x];
}
int board::at(point_t p) const {
    return m_cell[p.y][p.x];
}
int board::area() const { return m_area; }
int board::w() const { return m_size.x; }
int board::h() const { return m_size.y; }
void board::put(point_t p, int value) { m_cell[p.y][p.x] = value; }
bool board::is_new() const { return m_is_new; }


block::block() = default;
block::block(block_t const & a) {
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

point_t block::offset(flip_t f, rot_t r) const { return m_offset[f][r]; }
point_t block::size(flip_t f, rot_t r) const { return m_size[f][r]; }
point_t block::offset(placement_t const & p) const { assert (p.used); return offset(p.f,p.r); }
point_t block::size(placement_t const & p) const { assert (p.used); return size(p.f,p.r); }
int block::area() const { return m_area; }
int block::w(flip_t f, rot_t r) const { return size(f,r).x; }
int block::h(flip_t f, rot_t r) const { return size(f,r).y; }
int block::w(placement_t const & p) const { return w(p.f,p.r); }
int block::h(placement_t const & p) const { return h(p.f,p.r); }
bool block::at(flip_t f, rot_t r, point_t p) const {
    return m_cell[f][r][p.y][p.x];
}
bool block::at(placement_t const & p, point_t q) const {
    return at(p.f, p.r, q - p.p);
}
bool block::is_duplicated(flip_t f, rot_t r) const { return m_duplicated[f][r]; }
std::vector<point_t> const & block::stones(flip_t f, rot_t r) const { return m_stones[f][r]; }
std::vector<point_t> block::stones(placement_t const & p) const {
    std::vector<point_t> qs = m_stones[p.f][p.r];
    for (auto && q : qs) q = q + p.p;
    return qs;
}


bool is_intersect(board & brd, block const & blk, placement_t const & p) {
    for (auto q : blk.stones(p)) {
        if (not is_on_board(q)) return true;
        if (brd.at(q)) return true;
    }
    return false;
}

bool is_puttable(board & brd, block const & blk, placement_t const & p) {
    if (is_intersect(brd, blk, p)) return false;
    if (brd.is_new()) {
        return true;
    } else {
        for (auto q : blk.stones(p)) {
            repeat (i,4) {
                point_t r = q + dp[i];
                if (not is_on_board(r)) continue;
                if (brd.at(r) >= 2) return true; // is a stone
            }
        }
        return false;
    }
}

/**
 * @pre 置ける is_puttableが真を返す
 * @attention 何も確認しないことに注意
 * @param brd
 *   変更される
 * @param value
 *   n番目の石を置くと: 2+n
 *   戻す: 0
 */
void put_stone(board & brd, block const & blk, placement_t const & p, int value) {
    for (auto q : blk.stones(p)) brd.put(q, value);
}

