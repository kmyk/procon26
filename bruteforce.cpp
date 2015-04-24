#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include "utility/type/ll.hpp"
#include "utility/range.hpp"
using namespace std;

constexpr int board_size = 32;
constexpr int block_size = 8;
enum flip_t { H, T };
enum rot_t { R0, R90, R180, R270 };
struct input_t {
    vector<vector<bool> > board;
    vector<vector<vector<bool> > > blocks;
};
struct placement_t {
    bool used;
    int x, y;
    flip_t f;
    rot_t r;
};
const placement_t ignore_placement = { false };
struct output_t {
    vector<placement_t> ps;
    int score;
};

ostream & operator << (ostream & output, flip_t a) { return output << (a == H ? 'H' : 'T'); }
ostream & operator << (ostream & output, rot_t a)  { return output << (a == R0 ? 0 : a == R90 ? 90 : a == R180 ? 180 : 270); }
input_t input() {
    vector<vector<bool> > board(board_size, vector<bool>(board_size));
    for (int y : range(board_size)) {
        for (int x : range(board_size)) {
            char c; cin >> c;
            board[y][x] = c == '1';
        }
    }
    int n; cin >> n;
    vector<vector<vector<bool> > > blocks(n);
    for (int i : range(n)) {
        vector<vector<bool> > b(block_size, vector<bool>(block_size));
        for (int y : range(block_size)) {
            for (int x : range(block_size)) {
                char c; cin >> c;
                b[y][x] = c == '1';
            }
        }
        blocks[i] = b;
    }
}

vector<vector<bool> > shrink(vector<vector<bool> > const & board, bool a_default) {
    int h = board.size();
    int w = board[0].size();
    while (true) {
        bool dummy = true;
        for (int x : range(w)) {
            if (board[h-1][x] != a_default) {
                dummy = false;
                break;
            }
        }
        if (not dummy) break;
        h -= 1;
    }
    while (true) {
        bool dummy = true;
        for (int y : range(h)) {
            if (board[y][w-1] != a_default) {
                dummy = false;
                break;
            }
        }
        if (not dummy) break;
        w -= 1;
    }
    vector<vector<bool> > result(h, vector<bool>(w));
    for (int y : range(h)) for (int x : range(w)) result[y][x] = board[y][x];
    return result;
}
output_t dfs(vector<vector<bool> > board, int i, vector<vector<vector<bool> > > const & blocks) {
    int h = board.size();
    int w = board[0].size();
    if (i == blocks.size()) {
        int score = 0;
        for (int y : range(h)) {
            for (int x : range(w)) {
                if (not board[y][x]) score += 1;
            }
        }
        return (output_t){ {}, score };
    }
    output_t result = dfs(board, i+1, blocks);
    result.ps.push_back(ignore_placement);
    for (int y : range(h)) {
        for (int x : range(w)) {
            for (flip_t f : {H,T}) {
                for (rot_t r : {R0,R90,R180,R270}) {
                }
            }
        }
    }
    return result;
}
output_t solve(vector<vector<bool> > board, vector<vector<vector<bool> > > blocks) {
    board = shrink(board, true);
    for (auto & block : blocks) block = shrink(block, false);
    return dfs(board, 0, blocks);
}
void output(output_t const & output) {
    for (auto p : output.ps) {
        if (p) cout << p->x << " " << p->y << " " << p->f << " " << p->r;
        cout << endl;
    }
}

int main() {
    ios_base::sync_with_stdio(false);
    auto p = input();
    output(solve(p.board,p.blocks));
    return 0;
}
