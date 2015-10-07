#include <iostream>
#include <csignal>
#include "procon26.hpp"
#include "exact.hpp"
#include "forward.hpp"

using namespace std;

output_t g_provisional_result; // externed

void signal_handler(int param) {
    cerr << "*** signal " << param << " caught ***" << endl;
    cout << g_provisional_result;
    cerr << "***" << endl;
    exit(param);
}

#ifdef SOLVER
int main() {
    ios_base::sync_with_stdio(false);
    input_t a; cin >> a;
    int n = a.blocks.size();
    board brd = board(a.board);
    vector<block> blks(n); repeat (i,n) blks[i] = block(a.blocks[i]);
    signal(SIGINT, &signal_handler);
    cout << (output_t) { SOLVER(brd, blks) };
    signal(SIGINT, SIG_DFL);
    return 0;
}
#endif
