#include "procon26.hpp"
#include <stack>
#include <algorithm>

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
template <typename T, int N, typename F>
void shrink_cell(T const (& a)[N][N], point_t & offset, point_t & size, F pred) {
    int & x = offset.x;
    int & y = offset.y;
    int & w = size.x;
    int & h = size.y;
    shrink_cell_helper(h, [&](int dy){ return pred(a[y+dy ][x    ]); }, [&](){ x += 1; w -= 1; });
    shrink_cell_helper(w, [&](int dx){ return pred(a[y    ][x+dx ]); }, [&](){ y += 1; h -= 1; });
    shrink_cell_helper(h, [&](int dy){ return pred(a[y+dy ][x+w-1]); }, [&](){         w -= 1; });
    shrink_cell_helper(w, [&](int dx){ return pred(a[y+h-1][x+dx ]); }, [&](){         h -= 1; });
}
template <typename T, int N>
void shrink_cell(T const (& a)[N][N], point_t & offset, point_t & size, T target) {
    shrink_cell(a, offset, size, [&](T it) -> bool { return it != target; });
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
    copy_cell_map(a.a, m_cell, [](bool x) -> int { return x; });
    shrink();
    update();
}
void board::shrink() {
    m_offset = { 0, 0 };
    m_size = { N, N };
    shrink_cell(m_cell, m_offset, m_size, 1);
    repeat (y,board_size) {
        m_skips[y][board_size-1] = 1;
        for (int x = board_size - 2; x >= 0; -- x) {
            m_skips[y][x] = m_cell[y][x+1] == 0 ? 1 : m_skips[y][x+1] + 1;
        }
    }
    m_stone_offset = { 0, 0 };
    m_stone_size = { N, N };
    shrink_cell(m_cell, m_stone_offset, m_stone_size, [](int n) -> bool { return n <= 1; });
}
void board::update() {
    m_area = area_cell(m_cell, 0);
    int obstacles = area_cell(m_cell, 1);
    m_stone_area = N * N - (m_area + obstacles);
}

point_t board::offset() const { return m_offset; }
point_t board::size() const { return m_size; }
point_t board::stone_offset() const { return m_stone_offset; }
point_t board::stone_size() const { return m_stone_size; }
int board::at_local(point_t p) const {
    point_t q = offset();
    return m_cell[p.y+q.y][p.x+q.x];
}
int board::at(point_t p) const {
    return m_cell[p.y][p.x];
}
int board::area() const { return m_area; }
int board::stone_area() const { return m_stone_area; }
int board::w() const { return m_size.x; }
int board::h() const { return m_size.y; }
bool board::is_new() const { return m_stone_area == 0; }
int board::skip(point_t p) const { return m_skips[p.y][p.x]; }

bool board::is_intersect(block const & blk, placement_t const & p) const {
    return is_intersect(blk, p, nullptr);
}
bool board::is_intersect(block const & blk, placement_t const & p, int *skipp) const {
    repeat (i, int(blk.stones(p).size())) {
        point_t q = blk.stones(p.f,p.r)[i] + p.p;
        if (q.y < 0 or board_size <= q.y or board_size <= q.x) {
            if (skipp) *skipp = board_size;
            return true;
        }
        if (q.x < 0) {
            if (skipp) *skipp = - q.x;
            return true;
        }
        if (at(q)) {
            if (skipp) *skipp = std::max(skip(q), blk.skips(p.f,p.r)[i]);
            return true;
        }
    }
    if (skipp) *skipp = 1;
    return false;
}

bool board::is_puttable(block const & blk, placement_t const & p) const {
    return is_puttable(blk, p, nullptr);
}

bool board::is_puttable(block const & blk, placement_t const & p, int *skipp) const {
    if (is_intersect(blk, p, skipp)) return false;
    if (is_new()) {
        return true;
    } else {
        for (auto const & q : blk.stones(p.f,p.r)) {
            repeat (i,4) {
                point_t r = (q + p.p) + dp[i];
                if (not is_on_board(r)) continue;
                if (at(r) >= 2) return true; // is a stone
            }
        }
        return false;
    }
}

void board::put(point_t p, int value) {
    m_cell[p.y][p.x] = value;
}
/**
 * @pre 置ける is_puttableが真を返す
 * @attention 何も確認しないことに注意
 * @param value
 *   n番目の石を置くと: 2+n
 *   戻す: 0
 */
void board::put(block const & blk, placement_t const & p, int value) {
    for (auto const & q : blk.stones(p.f,p.r)) put(q + p.p, value);
}

std::vector<board> board::split() const {
    std::vector<board> result;
    bool used[board_size][board_size] = {};
    repeat_from (y, offset().y, offset().y+size().y) {
        repeat_from (x, offset().x, offset().x+size().x) {
            if (m_cell[y][x] == 0 and not used[y][x]) {
                board brd;
                copy_cell_map(m_cell, brd.m_cell, [](int x) -> int { return std::max(1, x); });
                std::stack<point_t> stk;
                stk.push({ x, y });
                while (not stk.empty()) {
                    point_t p = stk.top(); stk.pop();
                    if (used[p.y][p.x]) continue;
                    used[p.y][p.x] = true;
                    brd.m_cell[p.y][p.x] = 0;
                    repeat (i,4) {
                        int ny = p.y + dy[i];
                        int nx = p.x + dx[i];
                        if (not is_on_board({ nx, ny })) continue;
                        if (m_cell[ny][nx] == 0 and not used[ny][nx]) {
                            stk.push({ nx, ny });
                        }
                    }
                }
                brd.shrink();
                brd.update();
                result.push_back(brd);
            }
            x += m_skips[y][x] - 1;
        }
    }
    return result;
}


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
            // skipが効きやすく
            std::sort(m_stones[f][r].begin(), m_stones[f][r].end(), [](point_t const & a, point_t const & b) {
                return std::make_pair(- a.x, - a.y) < std::make_pair(- b.x, - b.y);
            });
        }
    }
    for (flip_t f : { H, T }) {
        for (rot_t r : { R0, R90, R180, R270 }) {
            m_skips[f][r].resize(m_stones[f][r].size());
            repeat (i, int(m_stones[f][r].size())) {
                point_t p = m_stones[f][r][i];
                m_skips[f][r][i] = 0;
                while (is_on_board(p) and m_cell[f][r][p.y][p.x]) {
                    p.x -= 1;
                    m_skips[f][r][i] += 1;
                }
            }
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
std::vector<int> const & block::skips(flip_t f, rot_t r) const { return m_skips[f][r]; }


placement_t initial_placement(block const & blk, point_t const & lp, flip_t f, rot_t r) {
    point_t p = lp - blk.size(f, r) - blk.offset(f, r);
    return (placement_t) { true, p, f, r };
}
placement_t initial_placement(block const & blk, point_t const & lp) {
    return initial_placement(blk, lp, H, R0);
}
bool next_placement(placement_t & p, block const & blk, point_t const & lp, point_t const & rp) {
    p.p.x += 1;
    if (p.p.x >= rp.x + 1 - blk.offset(p.f, p.r).x) {
        p.p.x = initial_placement(blk, lp, p.f, p.r).p.x;
        p.p.y += 1;
        if (p.p.y >= rp.y + 1 - blk.offset(p.f, p.r).y) {
            p.r = rot90(p.r);
            if (p.r == R0) {
                p.f = flip(p.f);
                if (p.f == H) {
                    p.p = initial_placement(blk, lp, p.f, p.r).p;
                    return false;
                }
            }
            p.p = initial_placement(blk, lp, p.f, p.r).p;
        }
    }
    return true;
}

void update_bounding_box(board const & brd, block const & blk, placement_t const & p, point_t const & lp, point_t const & rp, point_t *nlp, point_t *nrp) {
    *nlp = p.p + blk.offset(p);
    *nrp = p.p + blk.offset(p) + blk.size(p);
    if (not brd.is_new()) { // 古いやつと合成
        *nlp = pwmin(*nlp, lp);
        *nrp = pwmax(*nrp, rp);
    }
}
