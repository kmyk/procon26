#include "procon26.hpp"

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
    shrink();
}
void board::shrink() {
    m_area = area_cell(m_cell, 0);
    int obstacles = area_cell(m_cell, 1);
    m_is_new = (m_area + obstacles == N * N) ? 1 : 0;
    shrink_cell(m_cell, m_offset, m_size, 1);
}

point_t board::offset() const { return m_offset; }
point_t board::size() const { return m_size; }
int board::at(point_t p) const {
    point_t q = offset();
    return m_cell[p.y+q.y][p.x+q.x];
}
int board::area() const { return m_area; }
int board::w() const { return m_size.x; }
int board::h() const { return m_size.y; }
void board::put(point_t p, int n) {
    point_t q = offset();
    int & it = m_cell[p.y+q.y][p.x+q.x];
    // assert (it == 0); // 確認入れたいがわざわざ確認なし版を作るのも面倒
    it = 2+n;
}
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
point_t block::local(flip_t f, rot_t r, point_t const & q) const {
    int ay = offset(f,r).y + q.y;
    int ax = offset(f,r).x + q.x;
    return { ax, ay };
}
bool block::at(flip_t f, rot_t r, point_t p) const {
    p = local(f,r,p);
    return m_cell[f][r][p.y][p.x];
}
point_t block::world(placement_t const & p, point_t const & q) const {
    assert (p.used);
    int ay = p.p.y + offset(p.f,p.r).y + q.y;
    int ax = p.p.x + offset(p.f,p.r).x + q.x;
    return { ax, ay };
}
bool block::is_duplicated(flip_t f, rot_t r) const { return m_duplicated[f][r]; }
std::vector<point_t> const & block::stones(flip_t f, rot_t r) const { return m_stones[f][r]; }
