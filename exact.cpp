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
        dfs(acc, 0, used);
#ifndef NEBUG
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

    void dfs(vector<placement_t> & acc, int score, bool (& used)[board_size][board_size]) {
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
        block const & blk = blks[l];
        for (flip_t f : { H, T }) {
            for (rot_t r : { R0, R90, R180, R270 }) {
                for (int y : irange(brd.y()-blk.h(f,r), brd.y()+brd.h())) {
                    for (int x : irange(brd.x()-blk.w(f,r), brd.x()+brd.w())) {
                        placement_t p = { true, { x, y }, f, r };
                        if (is_puttable(blk, p, used, score == 0)) {
                            put(blk, p, true, used);
                            acc.push_back(p);
                            dfs(acc, score+blk.area(), used);
                            acc.pop_back();
                            put(blk, p, false, used);
                        }
                    }
                }
            }
        }
        acc.push_back({ false });
        dfs(acc, score, used);
        acc.pop_back();
    }
};

int main() {
    ios_base::sync_with_stdio(false);
    input_t a; cin >> a;
    cout << solver()(a);
    return 0;
}
