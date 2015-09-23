#include <iostream>
#include "procon26.hpp"
#include "exact.hpp"

using namespace std;

int main() {
    ios_base::sync_with_stdio(false);
    input_t a; cin >> a;
    cout << (output_t) { exact(a).placement };
    return 0;
}
