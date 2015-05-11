#include <iostream>
#include <vector>
#include <set>
#include <cassert>
#include <cstdint>
// #include <boost/geometry.hpp>
#include <boost/range/irange.hpp>
namespace boost {
    template<class Integer>
    iterator_range< range_detail::integer_iterator<Integer> >
    irange(Integer last) {
        return boost::irange(static_cast<Integer>(0), last);
    }
}

constexpr int board_size = 32;
constexpr int block_size = 8;

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
    using namespace boost;
    for (int y : irange(board_size)) {
        for (int x : irange(board_size)) {
            char c; input >> c;
            a.a[y][x] = c == '0';
        }
    }
    return input;
}
inline std::istream & operator >> (std::istream & input, block_t & a)  {
    using namespace boost;
    for (int y : irange(block_size)) {
        for (int x : irange(block_size)) {
            char c; input >> c;
            a.a[y][x] = c == '1';
        }
    }
    return input;
}
inline std::istream & operator >> (std::istream & input, input_t & a)  {
    using namespace boost;
    input >> a.board;
    // > 石の個数は 1 個以上，256 個以下です。
    int n; input >> n;
    assert (1 <= n and n <= 256);
    a.blocks.resize(n);
    for (int i : irange(n)) input >> a.blocks[i];
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

template <int N>
void copy_cell(bool const (& src)[N][N], bool (& dst)[N][N]) {
    using namespace boost;
    for (int y : irange(N)) {
        for (int x : irange(N)) {
            dst[y][x] = src[y][x];
        }
    }
}
template <int N>
void copy_cell_rot90(bool const (& src)[N][N], bool (& dst)[N][N]) {
    using namespace boost;
    for (int y : irange(N)) {
        for (int x : irange(N)) {
            // 8×8 のマス全体を時計回りに 90 度，180 度，270 度回転させることができる。
            dst[x][N-1-y] = src[y][x];
        }
    }
}
template <int N>
void copy_cell_flip(bool const (& src)[N][N], bool (& dst)[N][N]) {
    using namespace boost;
    for (int y : irange(N)) {
        for (int x : irange(N)) {
            // > 裏返した後は，与えられたときの配置と左右に反転しているものとする。
            dst[y][N-x-1] = src[y][x];
        }
    }
}
template <class F, class G>
void shrink_cell_helper(int n, F pred, G update) {
    using namespace boost;
    while (true) {
        for (int i : irange(n)) {
            if (pred(i)) return; // if something found, exit
        }
        update(); // shrink
    }
}
template <int N>
void shrink_cell(bool (& a)[N][N], point_t & offset, point_t & size) {
    int & x = offset.x;
    int & y = offset.y;
    int & w = size.x;
    int & h = size.y;
    shrink_cell_helper(h, [&](int dy){ return a[y+dy ][x    ]; }, [&](){ x += 1; w -= 1; });
    shrink_cell_helper(w, [&](int dx){ return a[y    ][x+dx ]; }, [&](){ y += 1; h -= 1; });
    shrink_cell_helper(h, [&](int dy){ return a[y+dy ][x+w-1]; }, [&](){         w -= 1; });
    shrink_cell_helper(w, [&](int dx){ return a[y+h-1][x+dx ]; }, [&](){         h -= 1; });
}
template <int N>
int area_cell(bool const (& a)[N][N]) {
    using namespace boost;
    int n = 0;
    for (int y : irange(N)) {
        for (int x : irange(N)) {
            if (a[y][x]) {
                n += 1;
            }
        }
    }
    return n;
}
template <int N>
std::vector<point_t> collect_cell(bool const (& a)[N][N], point_t const & offset, point_t const & size) {
    std::vector<point_t> result;
    for (int y : boost::irange(size.y)) {
        for (int x : boost::irange(size.x)) {
            if (a[offset.y+y][offset.x+x]) result.push_back({ x, y });
        }
    }
    return result;
}

uint64_t signature_block(bool const (& a)[block_size][block_size], point_t const & offset) {
    using namespace boost;
    static_assert (block_size == 8, "");
    uint64_t z = 0; // 8*8 == 64
    for (int y : irange(offset.y, 8)) {
        uint64_t w = 0;
        for (int x : irange(offset.x, 8)) {
            w = w << 1;
            if (a[y][x]) w = w | 1;
        }
        w = w << offset.x;
        z = (z << 8) | w;
    }
    z = z << (8 * offset.y);
    return z;
}

class board {
public:
    static constexpr int N = board_size;
private:
    bool m_cell[N][N]; // m_cells?
    point_t m_offset;
    point_t m_size;
    int m_area;
public:
    board() = default;
    board(board_t const & a) {
        m_offset = { 0, 0 };
        m_size = { N, N };
        copy_cell(a.a, m_cell);
        m_area = area_cell(a.a);
    }

public:
    bool const (& cell() const) [N][N] { return m_cell; }
    point_t offset() const { return m_offset; }
    point_t size() const { return m_size; }
    int area() const { return m_area; }
    int x() const { return m_offset.x; }
    int y() const { return m_offset.y; }
    int w() const { return m_size.x; }
    int h() const { return m_size.y; }
};

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
        m_area = area_cell(a.a);
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
                shrink_cell(m_cell[f][r], m_offset[f][r], m_size[f][r]);
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
