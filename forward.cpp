#include "forward.hpp"
#include <vector>
#include <algorithm>
#include <set>
#include <memory>
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
    int dead_stone;
    vector<placement_t> plc;
    int bix; // current block
};
typedef shared_ptr<photon_t> photon_ptr;

double evaluate(photon_t const & a) {
    double p = a.bix /(double) a.plc.size();
    double q = 1 - p;
    return
        - a.circumference * (8 * q)
        - a.score * (32 + 32 * p)
        - a.dead_area * (8 * q)
        - a.dead_stone * (16 * q);
}

// larger iff better
bool operator < (photon_t const & a, photon_t const & b) {
    return evaluate(a) < evaluate(b);
}

bool photon_ptr_comparator(photon_ptr const & a, photon_ptr const & b) {
    return *a < *b;
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
    assert (pho.score >= 0);
    repeat_from (i, pho.bix, blks.size()) if (not pho.plc[i].used) {
        placement_t const & p = ys[j ++];
        if (not p.used) continue;
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
    assert (pho.score >= 0);
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
        vector<photon_ptr> beam;
        for (auto && brd : a_brd.split()) {
            photon_ptr ppho = make_shared<photon_t>();
            photon_t & pho = *ppho;
            pho.brds.push_back(brd);
            pho.score = a_brd.area();
            pho.circumference = 0; // 相対的なもののみ気にする
            pho.dead_area = a_brd.area() - brd.area();
            pho.plc.resize(n, { false });
            pho.bix = 0;
            beam.push_back(ppho);
        }
int nthbeam = 0;
        vector<photon_ptr> next;
        while (not beam.empty()) {
            for (auto && ppho : beam) {
                photon_t const & pho = *ppho;
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
                        photon_ptr npho = make_shared<photon_t>();
                        *npho = pho;
                        npho->bix = bix + 1;
                        npho->dead_stone += blk.area();
                        next.push_back(npho);
                    }
                    int l = pho.brds.size();
                    repeat (bjx, l) {
                        board const & brd = pho.brds[bjx];
                        placement_t p = initial_placement(blk, brd.stone_offset());
                        do {
                            int skip;
                            if (brd.is_puttable(blk, p, &skip)) {
                                photon_ptr pnpho = make_shared<photon_t>();
                                *pnpho = pho;
                                photon_t & npho = *pnpho;
                                npho.plc[bix] = p;
                                npho.bix = bix + 1;
                                npho.score -= blk.area();
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
#if 0
                                if (bjx != l - 1) npho.brds[bjx] = npho.brds[l - 1];
                                npho.brds.pop_back();
                                for (auto && it : nbrd.split()) {
                                    if (it.area() <= exact_limit) {
                                        exact(npho, it, blks);
                                    } else {
                                        npho.brds.push_back(it);
                                    }
                                }
#else
                                npho.brds[bjx] = nbrd;
#endif
                                next.push_back(pnpho);
                            }
                            p.p.x += skip - 1;
                        } while (next_placement(p, blk, brd.stone_offset(), brd.stone_offset() + brd.stone_size()));
                    }
                }
                if (beam_width * 10 < next.size()) {
                    sort(next.rbegin(), next.rend(), &photon_ptr_comparator);
                    next.resize(beam_width);
                }
            }
            sort(next.rbegin(), next.rend(), &photon_ptr_comparator);
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
