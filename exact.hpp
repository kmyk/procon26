#pragma once
#include "procon26.hpp"

struct result_exact_t {
    std::vector<placement_t> placement; // 未使用の石情報も含んでいることに注意
    int score; // 残り面積
};

/**
 * @attention
 *   > (2) 敷き詰めた石の個数（石の個数が少ないチームが上位）
 *   であるが得点のみの意味での厳密解であることに注意
 */
result_exact_t exact(board const & brd, std::vector<block> const & blks);
