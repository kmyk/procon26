#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include "procon26.hpp"
using namespace std;
using namespace boost;

class solver {
    board brd;
    vector<block> blks;
    vector<placement_t> result;
    int highscore;
public:
    solver() = default;

public:
    output_t operator () (input_t const & a) {
        brd = board(a.board);
        blks.resize(a.blocks.size()); for (int i : irange(blks.size())) blks[i] = block(a.blocks[i]);
#ifndef NDEBUG
        cerr << "board size: " << brd.size() << endl;
        for (auto & blk : blks) cerr << "block size: " << blk.size(H,R0) << endl;
#endif
        result.clear();
        highscore = -1;
        vector<placement_t> acc;
        bool used[board_size][board_size] = {};
        dfs(acc, 0, used, -1,-1,-1,-1);
#ifndef NDEBUG
        assert (result.size() == a.blocks.size());
        assert (acc.size() == 0);
        for (int y : irange(board_size)) {
            for (int x : irange(board_size)) {
                assert (not used[y][x]);
            }
        }
#endif
        return { result };
    }

private:
    bool is_puttable(block const & blk, placement_t const & p, bool const (& used)[board_size][board_size], bool is_first) {
        bool connected = false;
        for (int dy : irange(blk.h(p))) {
            for (int dx : irange(blk.w(p))) {
                if (blk.at(p.f,p.r,{dx,dy})) {
                    point_t q = blk.world(p,{dx,dy});
                    int x = q.x;
                    int y = q.y;
                    if (not (0 <= y and y < board_size)) return false;
                    if (not (0 <= x and x < board_size)) return false;
                    if (used[y][x] or not brd.cell()[y][x]) return false;
                    if (not connected) {
                        connected = (y+1 < board_size and used[y+1][x])
                            or (0 <= y-1 and used[y-1][x])
                            or (x+1 < board_size and used[y][x+1])
                            or (0 <= x-1 and used[y][x-1]);
                    }
                }
            }
        }
        if (not is_first and not connected) return false;
        return true;
    }
    void put(block const & blk, placement_t const & p, bool v, bool (& used)[board_size][board_size]) {
        for (int dy : irange(blk.h(p))) {
            for (int dx : irange(blk.w(p))) {
                if (blk.at(p.f,p.r,{dx,dy})) {
                    point_t q = blk.world(p,{dx,dy});
                    used[q.y][q.x] = v;
                }
            }
        }
    }

    void dfs(vector<placement_t> & acc, int score, bool (& used)[board_size][board_size], int yl, int yr, int xl, int xr) {
        int l = acc.size();
        if (l == blks.size()) {
            if (highscore < score) {
                highscore = score;
                result = acc;
#ifndef NDEBUG
                cerr << "highscore: " << highscore << endl;
#endif
            }
            return;
        }
        bool is_first = score == 0;
        block const & blk = blks[l];
        if (is_first) {
            yl = brd.y();
            yr = brd.y() + brd.h();
            xl = brd.x();
            xr = brd.x() + brd.w();
        }
        for (flip_t f : { H, T }) {
            for (rot_t r : { R0, R90, R180, R270 }) {
                //   543210      [w=5]
                //   [w=5][ ... ])
                //   ^    ^      ^      => [xl-w, xr]
                // xl-w   xl    xr
                for (int y : irange(yl - blk.h(f,r), yr+1)) {
                    for (int x : irange(xl - blk.w(f,r), xr+1)) {
                        placement_t p = { true, { x, y }, f, r };
                        if (is_puttable(blk, p, used, is_first)) {
                            put(blk, p, true, used);
                            acc.push_back(p);
                            point_t q = blk.world(p,{0,0});
                            point_t s = blk.size(p);
                            int nyl = q.y;
                            int nyr = q.y + s.y;
                            int nxl = q.x;
                            int nxr = q.x + s.x;
                            if (not is_first) {
                                nyl = min(nyl, yl);
                                nyr = max(nyr, yr);
                                nxl = min(nxl, xl);
                                nxr = max(nxr, xr);
                            }
                            dfs(acc, score+blk.area(), used, nyl, nyr, nxl, nxr);
                            acc.pop_back();
                            put(blk, p, false, used);
                        }
                    }
                }
            }
        }
        acc.push_back({ false });
        dfs(acc, score, used, yl, yr, xl, xr);
        acc.pop_back();
    }
};

int main() {
    ios_base::sync_with_stdio(false);
    input_t a; cin >> a;
    cout << solver()(a);
    return 0;
}
