#include <iostream>
#include <fstream>
#include <sstream>
#include <csignal>
#include <boost/filesystem.hpp>
#include "procon26.hpp"
#include "exact.hpp"
#include "beam_search.hpp"

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
int g_best_score = 1000000007;;

void signal_handler(int param) {
    cerr << "*** signal " << param << " caught ***" << endl;
    cout << g_provisional_result;
    cerr << "***" << endl;
    export_to_file(g_provisional_result);
    cerr << g_best_score << endl;
    exit(param);
}

int main(int argc, char **argv) {
    ios_base::sync_with_stdio(false);
    input_t a; cin >> a;
    int n = a.blocks.size();
    board brd = board(a.board);
    vector<block> blks(n); repeat (i,n) blks[i] = block(a.blocks[i]);
    assert (argc == 3 or argc == 4);
    int BEAM_WIDTH = atoi(argv[1]);
    double BEAM_SEARCH_TIME = atof(argv[2]);
    bool IS_CHOKUDAI = 3 < argc ? atoi(argv[3]) : false;
    if (BEAM_SEARCH_TIME > 0.01) {
        clock_t start = clock();
        beam_search(brd, blks, BEAM_WIDTH, false);
        clock_t clock_per_width = (clock() - start) / BEAM_WIDTH;
        double sec_per_width = clock_per_width /(double) CLOCKS_PER_SEC;
        BEAM_WIDTH = min<int>(16384, (BEAM_SEARCH_TIME * 60 / sec_per_width - BEAM_WIDTH) * 0.95);
        cerr << "measured: " << sec_per_width << " sec/width" << endl;
    }
    cerr << "start with width: " <<  BEAM_WIDTH << endl;
    signal(SIGINT, &signal_handler);
    output_t b = { beam_search(brd, blks, BEAM_WIDTH, IS_CHOKUDAI) };
    signal(SIGINT, SIG_DFL);
    cout << b;
    export_to_file(b);
    cerr << g_best_score << endl;
    return 0;
}
