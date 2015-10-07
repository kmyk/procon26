#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
#include "common.hpp"
#include "procon26.hpp"
using namespace std;
using namespace boost;

output_t g_provisional_result; // dummy

BOOST_AUTO_TEST_SUITE(suite_block)

BOOST_AUTO_TEST_CASE(case_block_1) {
    block_t a = { {
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 1, 1, 0, 0, 0 },
        { 0, 0, 1, 1, 1, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    } };
    block b(a);
    BOOST_CHECK_EQUAL (b.offset(H, R0), ((point_t){ 2, 2 }));
    BOOST_CHECK_EQUAL (b.size(H, R0), ((point_t){ 3, 2 }));
    BOOST_CHECK_EQUAL (b.at(H, R0, b.offset(H, R0) + (point_t) { 0, 0 }), false);
    BOOST_CHECK_EQUAL (b.at(H, R0, b.offset(H, R0) + (point_t) { 1, 0 }), true);
    BOOST_CHECK_EQUAL (b.at(H, R0, b.offset(H, R0) + (point_t) { 0, 1 }), true);
    BOOST_CHECK_EQUAL (b.area(), 5);
    BOOST_CHECK_EQUAL (b.size(H, R90), ((point_t){ 2, 3 }));
    repeat (i, int(b.stones(H,R0).size())) {
        if (b.stones(H,R0)[i] == (point_t) { 4, 3 }) {
            BOOST_CHECK_EQUAL (b.skips(H,R0)[i], 3);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(suite_board)

BOOST_AUTO_TEST_CASE(case_board_1) {
    board_t a = { {
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1, },
        { 1,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1, },
        { 1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1, },
        { 1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1, },
        { 1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1, },
        { 1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1, },
        { 1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1, },
        { 1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1, },
        { 1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1, },
        { 1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1, },
        { 1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1, },
        { 1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
    } };
    board b(a);
    BOOST_CHECK_EQUAL (b.offset(), ((point_t){ 3, 4 }));
    BOOST_CHECK_EQUAL (b.size(), ((point_t){ 25, 12 }));
    BOOST_CHECK_EQUAL (b.at(b.offset() + (point_t) { 0, 0 }), false);
    BOOST_CHECK_EQUAL (b.at(b.offset() + (point_t) { 1, 0 }), false);
    BOOST_CHECK_EQUAL (b.at(b.offset() + (point_t) { 0, 1 }), false);
    BOOST_CHECK_EQUAL (b.at(b.offset() + (point_t) { 1, 1 }), true);
    BOOST_CHECK_EQUAL (b.area(), 25*12-1);
    BOOST_CHECK_EQUAL (b.skip({ 0, 4 }), 3);
}

BOOST_AUTO_TEST_CASE(case_board_split_1) {
    board_t a = { {
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,1,1,1, },
        { 1,1,1,1,1,1,0,0,1,0,1,1,0,0,1,0,0,0,0,0,1,0,0,1,1,1,1,0,1,1,1,1, },
        { 1,1,1,1,1,1,0,0,1,0,1,1,0,0,1,0,0,0,0,0,1,0,0,1,0,0,1,0,1,1,1,1, },
        { 1,1,1,1,1,1,0,0,1,0,0,0,0,0,1,0,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1, },
        { 1,1,1,1,1,1,0,0,1,0,1,1,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,0,0,1,0,0,0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,0,0,0,0,1,1,1,1, },
        { 1,1,1,1,1,1,0,0,1,0,0,0,0,0,1,1,1,1,1,1,1,1,1,0,1,0,0,0,1,1,1,1, },
        { 1,1,1,1,1,1,0,0,1,0,0,0,0,0,1,0,1,1,1,1,1,1,1,0,0,0,1,0,1,1,1,1, },
        { 1,1,1,1,1,1,0,0,1,0,1,0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,0,1,1,1,1, },
        { 1,1,1,1,1,1,0,0,1,0,0,0,0,0,1,0,1,1,1,1,1,1,1,0,0,0,1,0,1,1,1,1, },
        { 1,1,1,1,1,1,0,0,1,0,0,0,0,0,1,0,1,1,1,1,1,1,1,0,0,0,0,0,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, },
    } };
    board b(a);
    auto c = b.split();
    BOOST_CHECK_EQUAL (c.size(), 8);
}

BOOST_AUTO_TEST_SUITE_END()
