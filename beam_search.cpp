#include "beam_search.hpp"
#include <vector>
#include <algorithm>
#include <stack>
#include <set>
#include <iterator>
#include <unordered_set>
#include <memory>
#include <cassert>
#include "procon26.hpp"
#include "exact.hpp"
#ifdef USE_BEAM_SEARCH
#include "signal.hpp"
#endif
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
    int score; // or remaining_area
    int circumference;
    int dead_area;
    int dead_stone;
    int remaining_stone;
    vector<placement_t> plc;
    int bix; // current block
    int isolated[4];
};
typedef shared_ptr<photon_t> photon_ptr;

double evaluate(photon_t const & a) {
    double p = a.bix /(double) a.plc.size();
    double q = 1 - p;
    return
        - a.circumference * (8 * q)
        - a.score * (12 * p)
        - max(0.0, a.score - a.remaining_stone * 0.8) * 64
        - a.isolated[0] * 16 * (p + 0.3)
        - a.isolated[1] *  8 * (p + 0.3)
        - a.isolated[2] *  6 * (p + 0.3)
        - a.isolated[3] *  4 * (p + 0.3);
}

// larger iff better
bool operator < (photon_t const & a, photon_t const & b) {
    return evaluate(a) < evaluate(b);
}

bool photon_ptr_comparator(photon_ptr const & a, photon_ptr const & b) {
    return *a < *b;
}

class beam_search_solver {
    board brd; // immutable
    vector<block> blks; // immutable
    vector<placement_t> result;
    int highscore;
    const int beam_width;
public:
    explicit beam_search_solver(int a_beam_width)
            : beam_width(a_beam_width) {
        highscore = -1;
    }

public:
    vector<placement_t> operator () (board const & a_brd, vector<block> const & blks) {
        int n = blks.size();
        highscore = a_brd.area();
        vector<photon_ptr> beam;
        for (auto && brd : a_brd.split()) {
            photon_ptr ppho = make_shared<photon_t>();
            photon_t & pho = *ppho;
            pho.brd = brd;
            pho.score = a_brd.area();
            pho.circumference = 0; // 相対的なもののみ気にする
            pho.dead_area = a_brd.area() - brd.area();
            pho.remaining_stone = 0;
            for (auto && blk : blks) pho.remaining_stone += blk.area();
            pho.plc.resize(n, { false });
            pho.bix = 0;
            for (int & it : pho.isolated) it = 0;
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
#ifdef USE_BEAM_SEARCH
                    g_provisional_result = { result };
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
                        npho->remaining_stone -= blk.area();
                        next.push_back(npho);
                    }
                    bool is_just_used = false; // ぴったり嵌るような使われ方をしたか
                    placement_t p = initial_placement(blk, pho.brd.stone_offset());
                    do {
                        int skip;
                        if (pho.brd.is_puttable(blk, p, 2+bix, &skip)) {
                            photon_ptr pnpho = make_shared<photon_t>();
                            *pnpho = pho;
                            photon_t & npho = *pnpho;
                            npho.plc[bix] = p;
                            npho.bix = bix + 1;
                            npho.score -= blk.area();
                            npho.remaining_stone -= blk.area();
                            repeat (j, blk.area()) {
                                point_t q = blk.stones(p.f,p.r)[j] + p.p;
                                repeat (i,4) {
                                    auto r = q + dp[i];
                                    npho.circumference +=
                                        not is_on_board(r) ? -1 :
                                        npho.brd.at(r) == 0 ? 1 :
                                        -1;
                                }
                                npho.brd.put(q, 2+bix);
                            }
                            if (pho.circumference - npho.circumference == blk.circumference()) {
                                if (is_just_used) continue;
                                is_just_used = true;
                                if (1 <= blk.area() and blk.area() <= 4) {
                                    npho.isolated[blk.area() - 1] -= 1;
                                }
                            }
                            npho.brd.update();
                            vector<point_t> neighbors;
                            repeat (j, blk.area()) {
                                point_t q = blk.stones(p.f,p.r)[j] + p.p;
                                repeat (i,4) {
                                    auto r = q + dp[i];
                                    if (is_on_board(r) and npho.brd.at(r) == 0) {
                                        neighbors.push_back(r);
                                    }
                                }
                            }
                            set<point_t> used;
                            for (point_t q : neighbors) {
                                if (used.count(q)) continue;
                                set<point_t> current;
                                current.insert(q);
                                int n = 1;
                                stack<point_t> stk;
                                stk.push(q);
                                while (not stk.empty()) {
                                    point_t r = stk.top(); stk.pop();
                                    repeat (i,4) {
                                        auto s = r + dp[i];
                                        if (used.count(s)) {
                                            n = 1000000007;
                                            break;
                                        }
                                        if (not current.count(s) and is_on_board(s) and npho.brd.at(s) == 0) {
                                            current.insert(s);
                                            stk.push(s);
                                            n += 1;
                                        }
                                    }
                                    if (5 <= n) break;
                                }
                                if (1 <= n and n <= 4) npho.isolated[n-1] += 1;
                                copy(current.begin(), current.end(), inserter(used, used.begin()));
                            }
                            next.push_back(pnpho);
                        }
                        p.p.x += skip - 1;
                    } while (next_placement(p, blk, pho.brd.stone_offset(), pho.brd.stone_offset() + pho.brd.stone_size()));
                }
                if (beam_width * 10 < next.size()) {
                    sort(next.rbegin(), next.rend(), &photon_ptr_comparator);
                    next.resize(beam_width);
                }
            }
            sort(next.rbegin(), next.rend(), &photon_ptr_comparator);
            if (beam_width < next.size()) next.resize(beam_width);
            for (auto && ppho : next) ppho->brd.shrink();
            next.swap(beam);
            next.clear();
cerr << "beam " << (nthbeam ++) << " : " << beam.size() << endl;
repeat (i, min<int>(3, beam.size())) {
    cerr << beam[i]->brd;
    cerr << "score: " << beam[i]->score << endl;
    cerr << "evaluate: " << evaluate(*beam[i]) << endl;
    cerr << "circumference: " << beam[i]->circumference << endl;
}
        }
        return result;
    }
};

vector<placement_t> beam_search(board const & brd, std::vector<block> const & blks, int beam_width) {
    return beam_search_solver(beam_width)(brd, blks);
}

vector<placement_t> beam_search(board const & brd, std::vector<block> const & blks) {
    return beam_search(brd, blks, 1024);
}
