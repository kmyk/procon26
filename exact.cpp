#include "exact.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "procon26.hpp"
#ifdef USE_EXACT
#include "signal.hpp"
#endif
using namespace std;

class exact_solver {
    board brd; /// *MUTABLE*
    vector<block> blks; // immutable
    vector<placement_t> acc; // MUTABLE
    vector<placement_t> result;
    vector<int> rest_stone; // rest_stone[i] = max_stone - blks[0 .. i-1].area()
    int highscore; // complement, the area of used blocks
public:
    exact_solver() = default;

public:
    vector<placement_t> operator () (board const & a_brd, vector<block> const & a_blks) {
        blks = a_blks;
        int n = blks.size();
        rest_stone.resize(n); {
            int max_stone = 0;
            for (auto blk : blks) max_stone += blk.area();
            rest_stone[0] = max_stone;
            repeat_from (i,0,n-1) rest_stone[i+1] = rest_stone[i] - blks[i].area();
        }
        result.clear();
        highscore = a_brd.area();
        for (auto b_brd : a_brd.split()) {
            brd = b_brd;
            dfs(a_brd.area());
        }
#ifndef NDEBUG
        assert (result.size() <= n);
        assert (acc.size() == 0);
#endif
        result.resize(n, (placement_t){ false });
        return result;
    }

private:
    void dfs(int score) {
        int l = acc.size();
        if (l == blks.size()) {
            if (score < highscore) {
                highscore = score;
                result = acc;
#ifdef USE_EXACT
                g_provisional_result = { result };
                cerr << highscore << endl;
#endif
            }
            return;
        }
        if (highscore <= score - rest_stone[l]) return;
        brd.shrink();
        block const & blk = blks[l];
        placement_t p = initial_placement(blk, brd.stone_offset());
        do {
            int skip;
            if (brd.is_puttable(blk, p, 2+l, &skip)) {
                brd.put(blk, p, 2+l);
                brd.update();
                acc.push_back(p);
                dfs(score - blk.area());
                acc.pop_back();
                brd.put(blk, p, 0);
                brd.update();
            }
            p.p.x += skip - 1;
        } while (next_placement(p, blk, brd.stone_offset(), brd.stone_offset() + brd.stone_size()));
        acc.push_back({ false });
        dfs(score);
        acc.pop_back();
    }
};

vector<placement_t> exact(board const & brd, vector<block> const & blks) {
    return exact_solver()(brd, blks);
}
