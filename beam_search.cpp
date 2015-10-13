#include "beam_search.hpp"
#include <vector>
#include <algorithm>
#include <stack>
#include <set>
#include <map>
#include <iterator>
#include <unordered_set>
#include <memory>
#include <cassert>
#include "procon26.hpp"
#include "signal.hpp"
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

class beam_search_solver;
struct photon_t {
    board brd;
    int score; // or remaining_area
    int stone_count;
    int circumference;
    int dead_area;
    int dead_stone;
    int remaining_stone;
    vector<placement_t> plc;
    int bix; // current block
    int isolated[4];
    beam_search_solver *solver;
};
typedef shared_ptr<photon_t> photon_ptr;

double evaluate(photon_t const & a);

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
    int least_stone;
    const int beam_width;
    int n;
    map<int,int> component_pack_table;
    array<vector<int>,2> remaining_small_blks;
    unordered_set<bitset<board_size * board_size> > *cache;
public:
    beam_search_solver(int a_beam_width, unordered_set<bitset<board_size * board_size> > *a_cache)
            : beam_width(a_beam_width),
              cache(a_cache) {
        n = -1;
        highscore = 1000000007;
        least_stone = 1000000007;
        component_pack_table[0 + 2 * 10] = 5;
        component_pack_table[0 + 6 * 10] = 5;
        component_pack_table[1 + 1 * 10] = 6;
        component_pack_table[1 + 3 * 10] = 6;
        component_pack_table[1 + 5 * 10] = 6;
        component_pack_table[3 + 3 * 10] = 6;
        component_pack_table[3 + 3 * 10] = 6;
        component_pack_table[0 + 3 * 10] = 7;
        component_pack_table[0 + 1 * 10] = 7;
        component_pack_table[1 + 4 * 10] = 7;
        component_pack_table[2 + 2 * 10] = 8;
    }

public:
    vector<placement_t> operator () (board const & a_brd, vector<block> const & blks) {
        n = blks.size();
        repeat (j,2) remaining_small_blks[j].resize(n);
        repeat_reverse (i,n-1) {
            repeat (j,2) {
                remaining_small_blks[j][i] = remaining_small_blks[j][i+1];
            }
            if (blks[i+1].area() <= 2) {
                remaining_small_blks[blks[i+1].area() - 1][i] += 1;
            }
        }
        highscore = a_brd.area();
        if (make_pair(a_brd.area(), 0) < make_pair(g_best_score, g_best_stone)) {
            g_best_score = a_brd.area();
            g_best_stone = 0;
        }
        vector<photon_ptr> beam;
        for (auto && brd : a_brd.split()) {
            photon_ptr ppho = make_shared<photon_t>();
            photon_t & pho = *ppho;
            pho.brd = brd;
            pho.score = a_brd.area();
            pho.stone_count = 0;
            pho.circumference = 0; // 相対的なもののみ気にする
            pho.dead_area = a_brd.area() - brd.area();
            pho.remaining_stone = 0;
            for (auto && blk : blks) pho.remaining_stone += blk.area();
            pho.plc.resize(n, { false });
            pho.bix = 0;
            for (int & it : pho.isolated) it = 0;
            pho.solver = this;
            beam.push_back(ppho);
        }
int nthbeam = 0;
        vector<photon_ptr> next;
        unordered_set<bitset<board_size*board_size> > used_board;
        while (not beam.empty()) {
            for (auto && ppho : beam) {
                photon_t const & pho = *ppho;
                if (make_pair(pho.score, pho.stone_count) < make_pair(highscore, least_stone)) {
                    highscore = pho.score;
                    least_stone = pho.stone_count;
                    result = pho.plc;
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
                    bool is_just_used = false; // ぴったり嵌るような使われ方をしたか // used_componentsと処理が被っている
                    set<vector<int> > used_components;
                    int pushed_count = 0;
                    placement_t p = initial_placement(blk, pho.brd.stone_offset());
                    do {
                        int skip;
                        if (pho.brd.is_puttable(blk, p, 2+bix, &skip)) {
                            photon_ptr pnpho = make_shared<photon_t>();
                            *pnpho = pho;
                            photon_t & npho = *pnpho;
                            npho.plc[bix] = p;
                            npho.stone_count += 1;
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
                            if (used_board.count(npho.brd.packed())) continue;
                            if (2 <= n and cache and cache->count(npho.brd.packed())) continue;
                            used_board.insert(npho.brd.packed());
                            set<point_t> looked_cell;
                            vector<int> components;
                            bool is_diverged = false; // there are some too large components
                            for (point_t q : neighbors) {
                                if (looked_cell.count(q)) continue;
                                set<point_t> current;
                                current.insert(q);
                                int n = 1;
                                stack<point_t> stk;
                                stk.push(q);
                                while (not stk.empty()) {
                                    point_t r = stk.top(); stk.pop();
                                    repeat (i,4) {
                                        auto s = r + dp[i];
                                        if (looked_cell.count(s)) {
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
                                if (1 <= n and n <= 4) {
                                    npho.isolated[n-1] += 1;
                                }
                                if (5 <= n) is_diverged = true;
                                if (not is_diverged) {
                                    if (n == 1 or n == 2) {
                                        components.push_back(n);
                                    } else if (n == 3) {
                                        vector<point_t> ps(current.begin(), current.end());
                                        components.push_back(3 + (cross(ps[1] - ps[0], ps[2] - ps[0]) == 0 ? 0 : 1));
                                    } else if (n == 4) {
                                        vector<point_t> ps(current.begin(), current.end());
                                        int sy = abs(ps[3].y + ps[2].y + ps[1].y - 3 * ps[0].y);
                                        int sx = abs(ps[3].x + ps[2].x + ps[1].x - 3 * ps[0].x);
                                        components.push_back(component_pack_table[min(sy, sx) + max(sy, sx) * 10]);
                                    }
                                }
                                copy(current.begin(), current.end(), inserter(looked_cell, looked_cell.begin()));
                            }
                            if (not is_diverged) {
                                sort(components.begin(), components.end());
                                if (used_components.count(components)) continue;
                                used_components.insert(components);
                            }
                            if (is_just_used) {
                                repeat (i,pushed_count) next.pop_back();
                                next.push_back(pnpho);
                                break; // justなら必ずそれを使うことにする
                            }
                            next.push_back(pnpho);
                            pushed_count += 1;
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
            if (cache) for (auto && ppho : next) cache->insert(ppho->brd.packed());
            next.swap(beam);
            next.clear();
            used_board.clear();
cerr << "beam " << (nthbeam ++) << " : " << beam.size() << endl;
if (cache) cerr << "cache " << cache->size() << endl;
            if (make_pair(highscore, least_stone) < make_pair(g_best_score, g_best_stone)) {
                g_provisional_result = { result };
                g_best_score = highscore;
                g_best_stone = least_stone;
                cout << g_provisional_result;
                cerr << g_best_score << " " << g_best_stone << endl;
            }
repeat (i, min<int>(3, beam.size())) {
    cerr << beam[i]->brd;
    cerr << "score: " << beam[i]->score << endl;
    cerr << "evaluate: " << evaluate(*beam[i]) << endl;
    cerr << "circumference: " << beam[i]->circumference << endl;
}
        }
        return result;
    }

    double evaluate(photon_t const & a) {
        double p = a.bix /(double) n;
        double q = 1 - p;
        return
            - a.circumference * (8 * q)
            - a.score * (12 * p)
            - max(0.0, a.score - a.remaining_stone * 0.8) * 64
            - max(0, a.isolated[0] - remaining_small_blks[0][a.bix]) * 32 * q
            - max(0, a.isolated[1] - remaining_small_blks[1][a.bix]) * 32 * q
            - a.isolated[0] * 16 * (q + 0.3)
            - a.isolated[1] *  8 * (q + 0.3)
            - a.isolated[2] *  6 * (q + 0.3)
            - a.isolated[3] *  4 * (q + 0.3)
            - a.stone_count * 0.00001;
    }
};

double evaluate(photon_t const & a) {
    return a.solver->evaluate(a);
}

vector<placement_t> beam_search(board const & brd, std::vector<block> const & blks, int beam_width, bool is_chokudai) {
    if (is_chokudai) {
        unordered_set<bitset<board_size * board_size> > cache;
        while (true) beam_search_solver(beam_width, &cache)(brd, blks);
    } else {
        return beam_search_solver(beam_width, nullptr)(brd, blks);
    }
}

vector<placement_t> beam_search(board const & brd, std::vector<block> const & blks) {
    return beam_search(brd, blks, 1024, false);
}
