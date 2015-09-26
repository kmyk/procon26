#include "forward.hpp"
#include <vector>
#include <algorithm>
#include <set>
#include <cassert>
#include "procon26.hpp"
#include "exact.hpp"
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
    vector<board> brds;
    int score;
    int circumference;
    int dead_area;
    vector<placement_t> plc;
    int bix; // current block
    // TODO: boardに持たせる
    // point_t lp;
    // point_t rp; // bounding box
};

double evaluate(photon_t const & a) {
    return - a.circumference - a.score * 64;
}

// larger iff better
bool operator < (photon_t const & a, photon_t const & b) {
    return evaluate(a) < evaluate(b);
}

constexpr int beam_width = 1024;
constexpr int exact_limit = 18;

void exact(photon_t & pho, board brd, vector<block> const & blks) {
    vector<block> xs;
    repeat_from (i, pho.bix, blks.size()) if (not pho.plc[i].used) {
        xs.push_back(blks[i]);
    }
    if (xs.empty()) return;
    vector<placement_t> ys = exact(brd, xs);
    int j = 0;
    repeat_from (i, pho.bix, blks.size()) if (not pho.plc[i].used) {
        placement_t const & p = ys[j ++];
        pho.plc[i] = p;
        block const & blk = blks[i];
        pho.score -= blk.area();
        for (auto q : blk.stones(p)) {
            repeat (i,4) {
                auto r = q + dp[i];
                pho.circumference +=
                    not is_on_board(r) ? 1 :
                    brd.at(q) == 0 ? 1 :
                    -1;
            }
            brd.put(q, 2+i);
        }
    }
    brd.update();
    pho.dead_area += brd.area();
}

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
        vector<photon_t> beam;
        for (auto && brd : a_brd.split()) {
            photon_t pho;
            pho.brds.push_back(brd);
            pho.score = a_brd.area();
            pho.circumference = 0; // 相対的なもののみ気にする
            pho.dead_area = a_brd.area() - brd.area();
            pho.plc.resize(n, { false });
            pho.bix = 0;
            // pho.lp = a_brd.offset();
            // pho.rp = a_brd.offset() + a_brd.size();
            beam.push_back(pho);
        }
int nthbeam = 0;
        vector<photon_t> next;
        while (not beam.empty()) {
            for (auto const & pho : beam) {
                if (pho.score < highscore) {
                    highscore = pho.score;
                    result = pho.plc;
#ifdef USE_FORWARD
                    cerr << highscore << endl;
#endif
                }
                int bix = pho.bix;
                while (bix < n and pho.plc[bix].used) bix += 1;
                if (bix < n) {
                    block const & blk = blks[bix];
                    {
                        photon_t npho = pho;
                        npho.bix = bix + 1;
                        next.push_back(npho);
                    }
                    int l = pho.brds.size();
                    repeat (bjx, l) {
                        board const & brd = pho.brds[bjx];
                        placement_t p = initial_placement(blk, /* pho.lp */ { 0, 0 });
                        do {
                            int skip;
                            if (brd.is_puttable(blk, p, &skip)) {
                                photon_t npho = pho;
                                npho.plc[bix] = p;
                                npho.bix = bix + 1;
                                npho.score -= blk.area();
                                // update_bounding_box(brd, blk, p, pho.lp, pho.rp, &npho.lp, &npho.rp);
                                board nbrd = brd;
                                for (auto q : blk.stones(p)) {
                                    repeat (i,4) {
                                        auto r = q + dp[i];
                                        npho.circumference +=
                                            not is_on_board(r) ? 1 :
                                            nbrd.at(q) == 0 ? 1 :
                                            -1;
                                    }
                                    nbrd.put(q, 2+bix);
                                }
                                nbrd.update();
                                if (bjx != l - 1) npho.brds[bjx] = npho.brds[l - 1];
                                npho.brds.pop_back();
                                for (auto && it : nbrd.split()) {
                                    if (it.area() <= exact_limit) {
                                        exact(npho, it, blks);
                                    } else {
                                        npho.brds.push_back(it);
                                    }
                                }
                                next.push_back(npho);
                            }
                            p.p.x += skip - 1;
                        } while (next_placement(p, blk, /* pho.lp, pho.rp */ { 0, 0 }, { board_size, board_size }));
                    }
                }
                if (beam_width * 10 < next.size()) {
                    sort(next.rbegin(), next.rend());
                    next.resize(beam_width);
                }
            }
            sort(next.rbegin(), next.rend());
            if (beam_width < next.size()) next.resize(beam_width);
            // for (auto && pho : next) pho.brd.shrink(); // split internally calls shrink
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
