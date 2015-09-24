#include "exact.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "procon26.hpp"
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
        brd = a_brd;
        blks = a_blks;
        int n = blks.size();
        rest_stone.resize(n); {
            int max_stone = 0;
            for (auto blk : blks) max_stone += blk.area();
            rest_stone[0] = max_stone;
            repeat_from (i,0,n-1) rest_stone[i+1] = rest_stone[i] - blks[i].area();
        }
        result.clear();
        highscore = brd.area();
        dfs(brd.area(), brd.offset(), brd.offset() + brd.size());
#ifndef NDEBUG
        assert (result.size() <= n);
        assert (acc.size() == 0);
#endif
        result.resize(n, (placement_t){ false });
        return result;
    }

private:
    /**
     * @param lp
     * @param rp 既に置いてある石のbounding box
     */
    void dfs(int score, point_t lp, point_t rp) {
        int l = acc.size();
        if (l == blks.size()) {
            if (score < highscore) {
                highscore = score;
                result = acc;
                cerr << highscore << endl;
            }
            return;
        }
        if (highscore <= score - rest_stone[l]) return;
        block const & blk = blks[l];
        placement_t p = initial_placement(blk, lp);
        do {
            int skip;
            if (is_puttable(brd, blk, p, &skip)) {
                point_t nlp = p.p + blk.offset(p); // 新たなbounding box
                point_t nrp = p.p + blk.offset(p) + blk.size(p);
                if (not brd.is_new()) { // 古いやつと合成
                    nlp = pwmin(nlp, lp);
                    nrp = pwmax(nrp, rp);
                }
                put_stone(brd, blk, p, 2+l);
                brd.update();
                acc.push_back(p);
                dfs(score - blk.area(), nlp, nrp);
                acc.pop_back();
                put_stone(brd, blk, p, 0);
                brd.update();
            }
            p.p.x += skip - 1;
        } while (next_placement(p, blk, lp, rp));
        acc.push_back({ false });
        dfs(score, lp, rp);
        acc.pop_back();
    }
};

vector<placement_t> exact(board const & brd, vector<block> const & blks) {
    return exact_solver()(brd, blks);
}
