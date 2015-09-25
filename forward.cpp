#include "forward.hpp"
#include <vector>
#include <algorithm>
#include <set>
#include <cassert>
#include "procon26.hpp"
using namespace std;

/**
 * 100000001 100000001
 * 101110101 100000001
 * 101010101 111111111
 * 111111111 111111111
 *
 * heuristikに考えて右の方がいいよね、境界線が滑らかで、連結で、
 * そこで境界の長さを考えてみたよ
 */

struct photon_t {
    board brd;
    int score;
    int circumference;
    vector<placement_t> plc;
    int bix; // current block
    vector<bool> used; // are blocks used
    point_t lp;
    point_t rp; // bounding box
};

double evaluate(photon_t const & a) {
    return - a.circumference - a.score * 64;
}

// larger iff better
bool operator < (photon_t const & a, photon_t const & b) {
    return evaluate(a) < evaluate(b);
}

constexpr int beam_width = 1024;

class forward_solver {
    board brd; // immutable
    vector<block> blks; // immutable
    vector<placement_t> result;
    int highscore;
public:
    forward_solver() = default;

public:
    vector<placement_t> operator () (board const & a_brd, vector<block> const & blks) {
        int n = blks.size();
        highscore = a_brd.area();
        vector<photon_t> beam; {
            photon_t pho;
            pho.brd = a_brd;
            pho.score = a_brd.area();
            pho.circumference = 0; // 相対的なもののみ気にする
            pho.plc.resize(n, { false });
            pho.bix = 0;
            pho.used.resize(n);
            pho.lp = a_brd.offset();
            pho.rp = a_brd.offset() + a_brd.size();
            beam.push_back(pho);
        }
int nthbeam = 0;
        vector<photon_t> next;
        while (not beam.empty()) {
            for (auto const & pho : beam) {
                if (pho.score < highscore) {
                    highscore = pho.score;
                    result = pho.plc;
                    cerr << highscore << endl;
                }
                photon_t npho = pho;
                while (npho.bix < n and npho.used[npho.bix]) npho.bix += 1;
                int bix = npho.bix;
                if (bix < n) {
                    block const & blk = blks[bix];
                    npho.bix += 1;
                    next.push_back(npho);
                    npho.used[bix] = true;
                    npho.score -= blk.area();
                    placement_t p = initial_placement(blk, pho.lp);
                    do {
                        int skip;
                        if (pho.brd.is_puttable(blk, p, &skip)) {
                            npho.lp = p.p + blk.offset(p); // 新たなbounding box
                            npho.rp = p.p + blk.offset(p) + blk.size(p);
                            if (not pho.brd.is_new()) { // 古いやつと合成
                                npho.lp = pwmin(npho.lp, pho.lp);
                                npho.rp = pwmax(npho.rp, pho.rp);
                            }
                            for (auto q : blk.stones(p)) {
                                repeat (i,4) {
                                    auto r = q + dp[i];
                                    npho.circumference +=
                                        not is_on_board(r) ? 1 :
                                        npho.brd.at(q) == 0 ? 1 :
                                        -1;
                                }
                                npho.brd.put(q, 2+bix);
                            }
                            npho.brd.update();
                            npho.plc[bix] = p;
                            next.push_back(npho);
                            npho.brd.put(blk, p, 0);
                            npho.brd.update();
                            npho.plc[bix] = { false };
                            npho.circumference = pho.circumference;
                        }
                        p.p.x += skip - 1;
                    } while (next_placement(p, blk, pho.lp, pho.rp));
                }
                if (beam_width * 10 < next.size()) {
                    sort(next.rbegin(), next.rend());
                    next.resize(beam_width);
                }
            }
            sort(next.rbegin(), next.rend());
            if (beam_width < next.size()) next.resize(beam_width);
            for (auto && pho : next) pho.brd.shrink();
            next.swap(beam);
            next.clear();
cerr << "beam " << (nthbeam ++) << " : " << beam.size() << endl;
        }
        return result;
    }
};

vector<placement_t> forward(board const & brd, std::vector<block> const & blks) {
    return forward_solver()(brd, blks);
}
