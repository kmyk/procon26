#include <cassert>
struct clazz {
    bool a[8][8];
    bool const (& foo() const) [8][8] {
        return a;
    }
};
int main() {
    clazz x;
    assert (x.foo()[1][0] == x.foo()[0][1]);
    return 0;
}
