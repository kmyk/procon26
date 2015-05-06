#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
typedef long long ll;
#include <boost/range/irange.hpp>
namespace boost {
    template<class Integer>
    iterator_range< range_detail::integer_iterator<Integer> >
    irange(Integer last) {
        return irange(0, last);
    }
}
using namespace std;
using namespace boost;

constexpr int board_size = 32;
constexpr int block_size = 8;
enum flip_t { H, T };
enum rot_t { R0, R90, R180, R270 };
struct board_t {
    bool cell[board_size][board_size];
    int x, y, w, h;
    const bool[board_size] (& operator [] (size_t i) const) { return cell[i]; }
    bool[board_size]       (& operator [] (size_t i))       { return cell[i]; }
};
struct block_t {
    bool cell[block_size][block_size];
    int x, y, w, h;
    int area;
    const bool[block_size] (& operator [] (size_t i) const) { return cell[i]; }
    bool[block_size]       (& operator [] (size_t i))       { return cell[i]; }
};
struct input_t {
    board_t board;
    vector<block_t> blocks;
};
struct placement_t {
    bool used;
    int x, y;
    flip_t f;
    rot_t r;
};
struct output_t {
    vector<placement_t> ps;
};
struct point_t { int x, y; };

ostream & operator << (ostream & output, flip_t a) { return output << (a == H ? 'H' : 'T'); }
ostream & operator << (ostream & output, rot_t a)  { return output << (a == R0 ? 0 : a == R90 ? 90 : a == R180 ? 180 : 270); }
ostream & operator << (ostream & output, placement_t a)  {
    if (a.used) output << a.x << " " << a.y << " " << a.f << " " << a.r;
    return output;
}
ostream & operator << (ostream & output, output_t const & a)  {
    for (auto p : a.ps) output << p << endl;
    return output;
}

istream & operator >> (istream & input, board_t & a)  {
    a.x = a.y = 0;
    a.w = a.h = board_size;
    for (int y : irange(a.h)) {
        for (int x : irange(a.w)) {
            char c; input >> c;
            a[y][x] = c == '0';
        }
    }
    return input;
}
istream & operator >> (istream & input, block_t & a)  {
    a.x = a.y = 0;
    a.w = a.h = block_size;
    a.area = 0;
    for (int y : irange(a.h)) {
        for (int x : irange(a.w)) {
            char c; input >> c;
            a[y][x] = c == '1';
            if (a[y][x]) a.area += 1;
        }
    }
    // > 石は 1 個以上かつ，16 個以下のブロックにより構成され，
    assert (1 <= a.area and a.area <= 16);
    return input;
}
istream & operator >> (istream & input, input_t & a)  {
    input >> a.board;
    int n; input >> n;
    a.blocks.resize(n);
    for (int i : irange(n)) input >> a.blocks[i];
    return input;
}

template <class T, class F, class G>
void shrink_impl(T & a, int n, F pred, G update) {
    while (true) {
        for (int i : irange(n)) {
            if (pred(i)) return; // if something found, exit
        }
        update(); // shrink
    }
}
template <class T>
T shrink(T a) {
    shrink_impl(a, a.h, [&](int dy){ return a[a.y+dy   ][a.x      ]; }, [&](){ a.x += 1; a.w -= 1; });
    shrink_impl(a, a.w, [&](int dx){ return a[a.y      ][a.x+dx   ]; }, [&](){ a.y += 1; a.h -= 1; });
    shrink_impl(a, a.h, [&](int dy){ return a[a.y+dy   ][a.x+a.w-1]; }, [&](){           a.w -= 1; });
    shrink_impl(a, a.w, [&](int dx){ return a[a.y+a.h-1][a.x+dx   ]; }, [&](){           a.h -= 1; });
    return a;
}

int block_height(block_t const & block, placement_t const & p) {
    return (p.r == R0 or p.r == R180) ? block.h : block.w;
}
int block_width(block_t const & block, placement_t const & p) {
    return (p.r == R0 or p.r == R180) ? block.w: block.h;
}
point_t block_local(block_t const & block, placement_t const & p, int y, int x) {
    int a = block.y;
    int b = block.x;
    int h = block.h;
    int w = block.w;
    // > 石操作の順番として，はじめに石の裏表を決め，そのあとに回転させる。
    switch (p.f) {
        case H:
            switch (p.r) {
                case R0:   return { b+  x,   a+  y   };
                case R90:  return { b+  y,   a+h-x-1 };
                case R180: return { b+w-x-1, a+h-y-1 };
                case R270: return { b+w-y-1, a+  x   };
            }
        case T:
            switch (p.r) {
                case R0:   return { b+w-x-1, a+  y   };
                case R90:  return { b+w-y-1, a+h-x-1 };
                case R180: return { b+  x,   a+h-y-1 };
                case R270: return { b+  y,   a+  x   };
            }
    }
}
bool block_at(block_t const & block, placement_t const & p, int y, int x) {
    point_t q = block_local(block,p,y,x);
    return block[q.y][q.x];
}
point_t block_offset(block_t const & block, placement_t const & p) {
    int a = block.y;
    int b = block.x;
    int h = block.h;
    int w = block.w;
    int s = block_size;
    switch (p.f) {
        case H:
            switch (p.r) {
                case R0:   return {   b,     a   };
                case R90:  return { s-a-h,   b   };
                case R180: return { s-b-w, s-a-h };
                case R270: return {   a,   s-b-w };
            }
        case T:
            switch (p.r) {
                case R0:   return { s-b-w,   a   };
                case R90:  return { s-a-h, s-b-w };
                case R180: return {   b,   s-a-h };
                case R270: return {   a,     b   };
            }
    }
}
point_t block_world(block_t const & block, placement_t const & p, int y, int x) {
    point_t q = block_offset(block,p);
    return { x + p.x + q.x, y + p.y + q.y };
}
bool is_block_puttable(block_t const & block, placement_t const & p, bool (& used)[board_size][board_size], board_t const & board, bool connected) {
    for (int dy : irange(block_height(block,p))) {
        for (int dx : irange(block_width(block,p))) {
            if (block_at(block,p,dy,dx)) {
                int y = block_world(block,p,dy,dx).y;
                int x = block_world(block,p,dy,dx).x;
                if (not (0 <= y and y < board_size)) return false;
                if (not (0 <= x and x < board_size)) return false;
                if (used[y][x] or not board[y][x]) return false;
                if (not connected) {
                    connected = (y+1 < board_size and used[y+1][x])
                        or (0 <= y-1 and used[y-1][x])
                        or (x+1 < board_size and used[y][x+1])
                        or (0 <= x-1 and used[y][x-1]);
                }
            }
        }
    }
    return connected;
}
void block_put(block_t const & block, placement_t const & p, bool (& used)[board_size][board_size], bool z) {
    for (int dy : irange(block_height(block,p))) {
        for (int dx : irange(block_width(block,p))) {
            if (block_at(block,p,dy,dx)) {
                int y = block_world(block,p,dy,dx).y;
                int x = block_world(block,p,dy,dx).x;
                used[y][x] = z;
            }
        }
    }
}

// TODO: should I make a class to pack the state?
void dfs(vector<placement_t> & acc, int score, bool (& used)[board_size][board_size], board_t const & board, vector<block_t> const & blocks, vector<placement_t> & result, int & highscore) {
    int l = acc.size();
    if (l == blocks.size()) {
        // when you have put all blocks
        if (highscore < score) {
            highscore = score;
            result = acc;
#ifndef NDEBUG
            cerr << "highscore: " << highscore << endl;
#endif
        }
        return;
    }
    block_t const & block = blocks[l];
    for (int s : irange(8)) {
        placement_t p;
        p.used = true;
        p.f = (flip_t)(s / 4); // R0, R90, R180, R270
        p.r =  (rot_t)(s % 4); // H or T
        // is this range correct?
        for (int y : irange(board.y-block_height(block,p), board.y+board.h)) {
            p.y = y;
            for (int x : irange(board.x-block_width(block,p), board.x+board.w)) {
                p.x = x;
                if (is_block_puttable(block, p, used, board, score == 0)) {
                    block_put(block, p, used, true);
                    acc.push_back(p);
                    dfs(acc, score + block.area, used, board, blocks, result, highscore);
                    acc.pop_back();
                    block_put(block, p, used, false);
                }
            }
        }
    }
    acc.push_back({ false });
    dfs(acc, score, used, board, blocks, result, highscore);
    acc.pop_back();
}

output_t solve(board_t board, vector<block_t> blocks) {
    board = shrink(board);
    for (auto & block : blocks) block = shrink(block);
#ifndef NDEBUG
    cerr << "board size: (" << board.w << ", " << board.h << ")" << endl;
    for (auto & block : blocks) cerr << "block size: (" << block.w << ", " << block.h << ")" << endl;
#endif
    vector<placement_t> acc;
    bool used[board_size][board_size] = {};
    vector<placement_t> result;
    int highscore = -1;
    dfs(acc, 0, used, board, blocks, result, highscore);
    assert (result.size() == blocks.size());
    return { result };
}

void test();
int main() {
    test();
    ios_base::sync_with_stdio(false);
    input_t a; cin >> a;
    cout << solve(a.board, a.blocks);
    return 0;
}

void test() {
    {
        block_t a = {};
        a[3][1] = true; //  12
        a[3][2] = true; // 3##
        a[4][2] = true; // 4 #
        a.y = 3; a.x = 1;
        a.h = 2; a.w = 2;
        a.area = 3;
        placement_t p = { true };
        p.x = 0;
        p.y = 0;
        p.f = H;
        p.r = R0;
        assert (block_at(a,p,0,0) == true);
        assert (block_at(a,p,0,1) == true);
        assert (block_at(a,p,1,0) == false);
        assert (block_at(a,p,1,1) == true);
        p.f = T;
        assert (block_at(a,p,0,0) == true);
        assert (block_at(a,p,0,1) == true);
        assert (block_at(a,p,1,0) == true);
        assert (block_at(a,p,1,1) == false);
        p.r = R90;
        assert (block_at(a,p,0,0) == true);
        assert (block_at(a,p,0,1) == true);
        assert (block_at(a,p,1,0) == false);
        assert (block_at(a,p,1,1) == true);
    }
    {
        block_t a = {};
        a[5][4] = true; //  345
        a[6][3] = true; // 5 #
        a[6][4] = true; // 6###
        a[6][5] = true;
        a.y = 5; a.x = 3;
        a.h = 2; a.w = 3;
        a.area = 4;
        placement_t p = { true };
        p.x = 0;
        p.y = 0;
        p.f = H;
        p.r = R0;
        assert (block_at(a,p,0,0) == false);
        assert (block_at(a,p,0,1) == true);
        assert (block_at(a,p,0,2) == false);
        assert (block_at(a,p,1,0) == true);
        assert (block_at(a,p,1,1) == true);
        assert (block_at(a,p,1,2) == true);
        p.r = R90;
        assert (block_at(a,p,0,0) == true);
        assert (block_at(a,p,0,1) == false);
        assert (block_at(a,p,1,0) == true);
        assert (block_at(a,p,1,1) == true);
        assert (block_at(a,p,2,0) == true);
        assert (block_at(a,p,2,1) == false);
    }
}
