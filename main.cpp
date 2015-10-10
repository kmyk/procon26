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
            oss << ".log-" << (i++) << ".txt";
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

int main() {
    ios_base::sync_with_stdio(false);
    input_t a; cin >> a;
    int n = a.blocks.size();
    board brd = board(a.board);
    vector<block> blks(n); repeat (i,n) blks[i] = block(a.blocks[i]);
    signal(SIGINT, &signal_handler);
#if defined USE_EXACT
    output_t b = { exact(brd, blks) };
#elif defined USE_FORWARD
#ifdef FORWARD_TIME
#define TEST_WIDTH 128
    clock_t start = clock();
    forward(brd, blks, TEST_WIDTH);
    clock_t clock_per_width = (clock() - start) / TEST_WIDTH;
    double sec_per_width = clock_per_width /(double) CLOCKS_PER_SEC;
    int width = min<int>(8192, (FORWARD_TIME * 60 / sec_per_width - TEST_WIDTH) * 0.95);
    cerr << "measured: " << sec_per_width << " sec/width" << endl;
#else
    int width = 2048;
#endif
    cerr << "start with width: " <<  width << endl;
    output_t b = { forward(brd, blks, width) };
#else
#error solver is not given
#endif
    signal(SIGINT, SIG_DFL);
    cout << b;
    export_to_file(b);
    return 0;
}
