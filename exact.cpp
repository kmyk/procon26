#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#ifndef NSIGNAL
#include <csignal>
#endif
#include "procon26.hpp"
using namespace std;

#ifndef NSIGNAL
output_t g_result;
void signal_handler(int param) {
    cerr << "*** signal " << param << " caught ***" << endl;
    cerr << g_result;
    cerr << "***" << endl;
    exit(param);
}
#endif

class solver {
    board brd;
    vector<block> blks;
    vector<placement_t> result;
    vector<int> rest_stone; // rest_stone[i] = max_stone - blks[0 .. i-1].area()
    int highscore; // complement, the area of used blocks
public:
    solver() = default;

public:
    pair<vector<placement_t>, int> solve(input_t const & a) {
        int n = a.blocks.size();
        brd = board(a.board);
        blks.resize(n); repeat (i,n) blks[i] = block(a.blocks[i]);
#ifndef NLOG
        cerr << "board size: " << brd.size() << endl;
        cerr << "board area: " << brd.area() << endl;
        cerr << "blocks number: " << blks.size() << endl;
        for (auto & blk : blks) cerr << "block size: " << blk.size(H,R0) << endl;
#endif
        rest_stone.resize(n); {
            int max_stone = 0;
            for (auto blk : blks) max_stone += blk.area();
            rest_stone[0] = max_stone;
            repeat_from (i,0,n-1) rest_stone[i+1] = rest_stone[i] - blks[i].area();
        }
        result.clear();
        highscore = brd.area();
        vector<placement_t> acc;
        bool used[board_size][board_size] = {};
#ifndef NSIGNAL
        signal(SIGINT, &signal_handler);
#endif
        dfs(acc, brd.area(), used, -1,-1,-1,-1);
#ifndef NSIGNAL
        signal(SIGINT, SIG_DFL);
#endif
#ifndef NDEBUG
        if (result.size() < a.blocks.size()) {
            result.resize(a.blocks.size(), (placement_t){ false });
        }
        assert (result.size() == a.blocks.size());
        assert (acc.size() == 0);
        repeat (y,board_size) {
            repeat (x,board_size) {
                assert (not used[y][x]);
            }
        }
#endif
        return make_pair(result, board_size*board_size - brd.area() - highscore);
    }

    output_t operator () (input_t const & a) {
        return { solve(a).first };
    }

private:
    bool is_puttable(block const & blk, placement_t const & p, bool const (& used)[board_size][board_size], bool is_first) {
        bool connected = false;
        for (auto q : blk.stones(p.f,p.r)) {
            point_t r = blk.world(p,q);
            int x = r.x;
            int y = r.y;
            if (not (0 <= y and y < board_size)) return false;
            if (not (0 <= x and x < board_size)) return false;
            if (used[y][x] or brd.at((point_t){ x - brd.offset().x, y - brd.offset().y })) return false;
            if (not connected) {
                connected = (y+1 < board_size and used[y+1][x])
                    or (0 <= y-1 and used[y-1][x])
                    or (x+1 < board_size and used[y][x+1])
                    or (0 <= x-1 and used[y][x-1]);
            }
        }
        if (not is_first and not connected) return false;
        return true;
    }
    void put(block const & blk, placement_t const & p, bool v, bool (& used)[board_size][board_size]) {
        for (auto q : blk.stones(p.f,p.r)) {
            point_t r = blk.world(p,q);
            used[r.y][r.x] = v;
        }
    }

    void dfs(vector<placement_t> & acc, int score, bool (& used)[board_size][board_size], int yl, int yr, int xl, int xr) {
        int l = acc.size();
        if (l == blks.size()) {
            if (score < highscore) {
                highscore = score;
                result = acc;
#ifndef NLOG
                cerr << "highscore: " << highscore << endl;
#endif
#ifndef NSIGNAL
                g_result = { result };
#endif
            }
            return;
        }
        if (highscore <= score - rest_stone[l]) return;
        bool is_first = score == brd.area();
#ifndef NLOG
        if (is_first) {
            cerr << "done: " << acc.size() << " / " << blks.size() << endl;
        }
#endif
        block const & blk = blks[l];
        if (is_first) {
            yl = brd.offset().y;
            yr = brd.offset().y + brd.h();
            xl = brd.offset().x;
            xr = brd.offset().x + brd.w();
        }
        for (flip_t f : { H, T }) {
            for (rot_t r : { R0, R90, R180, R270 }) {
                if (blk.is_duplicated(f,r)) continue;
                //   543210      [w=5]
                //   [w=5][ ... ])
                //   ^    ^      ^      => [xl-w, xr]
                // xl-w   xl    xr
                repeat_from (y, yl - blk.h(f,r), yr + 1) {
                    repeat_from (x, xl - blk.w(f,r), xr + 1) {
                        placement_t p = { true, { x - blk.offset(f,r).x, y - blk.offset(f,r).y }, f, r };
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
                            dfs(acc, score-blk.area(), used, nyl, nyr, nxl, nxr);
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

// まず問題にはならないと思われるが、
// > (2) 敷き詰めた石の個数（石の個数が少ないチームが上位）
// であるが得点のみの意味での厳密解であることに注意
int main() {
    ios_base::sync_with_stdio(false);
    input_t a; cin >> a;
    cout << solver()(a);
    return 0;
}
