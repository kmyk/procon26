#include <iostream>
#include <fstream>
#include <sstream>
#include <csignal>
#include <boost/filesystem.hpp>
#include "procon26.hpp"
#include "exact.hpp"
#include "forward.hpp"

using namespace std;

void export_to_file(output_t const & a) {
    string path; {
        int i = 0;
        do {
            ostringstream oss;
            oss << ".log-" << i << ".txt";
            path = oss.str();
        } while (boost::filesystem::exists(path));
    }
    ofstream fp(path, ios::out);
    fp << a;
}

output_t g_provisional_result; // externed

void signal_handler(int param) {
    cerr << "*** signal " << param << " caught ***" << endl;
    cout << g_provisional_result;
    cerr << "***" << endl;
    export_to_file(g_provisional_result);
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
    output_t b = { SOLVER(brd, blks) };
    signal(SIGINT, SIG_DFL);
    cout << b;
    export_to_file(b);
    return 0;
}
#endif
