#include "forward.hpp"
#include <vector>
#include <algorithm>
#include <set>
#include <cassert>
#include "procon26.hpp"
using namespace std;

struct photon_t {
    board brd;
    int score;
    vector<placement_t> plc;
    int bix; // current block
    vector<bool> used; // are blocks used
    point_t lp;
    point_t rp; // bounding box
};

// larger iff better
bool operator < (photon_t const & a, photon_t const & b) {
    return - a.score < - b.score;
}

constexpr int beam_width = 64;

class forward_solver {
    board brd; // immutable
    vector<block> blks; // immutable
    vector<placement_t> result;
    int highscore;
public:
    forward_solver() = default;

public:
    vector<placement_t> operator () (board const & brd, vector<block> const & blks) {
        int n = blks.size();
        highscore = brd.area();
        vector<photon_t> beam; {
            photon_t pho;
            pho.brd = brd;
            pho.score = brd.area();
            pho.plc.resize(n, { false });
            pho.bix = 0;
            pho.used.resize(n);
            pho.lp = brd.offset();
            pho.rp = brd.offset() + brd.size();
            beam.push_back(pho);
        }
        while (not beam.empty()) {
            vector<photon_t> next;
            for (auto const & pho : beam) {
                if (pho.score < highscore) {
                    highscore = pho.score;
cerr << highscore << endl;
                    result = pho.plc;
                }
                photon_t npho = pho;
                while (npho.bix < n and npho.used[npho.bix]) npho.bix += 1;
                if (pho.bix < n) {
                    block const & blk = blks[pho.bix];
                    int bix = npho.bix;
                    npho.bix += 1;
                    next.push_back(npho);
                    npho.used[bix] = true;
                    npho.score -= blk.area();
// exact.cpp からほぼこぴぺ
/* BEGIN */
for (flip_t f : { H, T }) {
    for (rot_t r : { R0, R90, R180, R270 }) {
        if (blk.is_duplicated(f,r)) continue;
        point_t ofs = pho.lp - blk.size(f,r); // offset
        repeat_from (y, ofs.y, pho.rp.y + 1) {
            repeat_from (x, ofs.x, pho.rp.x + 1) {
                point_t ltp = { x, y }; // left-top
                placement_t p = { true, ltp - blk.offset(f,r), f, r };
                if (is_puttable(pho.brd, blk, p)) {
                    npho.lp = blk.offset(p); // 新たなbounding box
                    npho.rp = blk.offset(p) + blk.size(p);
                    if (not brd.is_new()) { // 古いやつと合成
                        npho.lp = pwmin(npho.lp, pho.lp);
                        npho.rp = pwmax(npho.rp, pho.rp);
                    }
                    put_stone(npho.brd, blk, p, 2+bix);
                    npho.brd.update();
                    npho.plc[bix] = p;
                    next.push_back(npho);
                    put_stone(npho.brd, blk, p, 0);
                    npho.brd.update();
                    npho.plc[bix] = { false };
                }
            }
        }
    }
}
/* END */
                }
            }
            sort(next.rbegin(), next.rend());
            if (beam_width < next.size()) next.resize(beam_width);
            beam = next;
        }
        return result;
    }
};

vector<placement_t> forward(board const & brd, std::vector<block> const & blks) {
    return forward_solver()(brd, blks);
}
