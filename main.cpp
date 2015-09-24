#include <iostream>
#include "procon26.hpp"
#include "exact.hpp"
#include "forward.hpp"

using namespace std;

#ifdef SOLVER
int main() {
    ios_base::sync_with_stdio(false);
    input_t a; cin >> a;
    int n = a.blocks.size();
    board brd = board(a.board);
    vector<block> blks(n); repeat (i,n) blks[i] = block(a.blocks[i]);
    cout << (output_t) { SOLVER(brd, blks) };
    return 0;
}
#endif
