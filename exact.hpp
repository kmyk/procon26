#pragma once
#include "procon26.hpp"

/**
 * @attention
 *   > (2) 敷き詰めた石の個数（石の個数が少ないチームが上位）
 *   であるが得点のみの意味での厳密解であることに注意
 */
std::vector<placement_t> exact(board const & brd, std::vector<block> const & blks);
