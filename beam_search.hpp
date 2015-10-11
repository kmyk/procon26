#pragma once
#include "procon26.hpp"

std::vector<placement_t> beam_search(board const & brd, std::vector<block> const & blks, int beam_width, bool is_chokudai);
std::vector<placement_t> beam_search(board const & brd, std::vector<block> const & blks);
