#pragma once
#include <vector>
#include <set>
#include <cassert>
#include <cstdint>
#include "common.hpp"

/**
 * @brief 役割: 原点と大きさの概念と置いた石の情報の吸収
 */
class board {
public:
    static constexpr int N = board_size;
private:
    // pimplする？
    int m_cell[N][N];
    point_t m_offset;
    point_t m_size;
    int m_area;
    bool m_is_new;
public:
    board();
    board(board_t const & a);
    void shrink();

public:
    point_t offset() const;
    point_t size() const;
    /**
     * @return
     *   - 0: 何も置かれていない
     *   - 1: 障害物
     *   - 2+n: n番目の石
     */
    int at(point_t p) const;
    int area() const;
    int w() const;
    int h() const;
    /**
     * @attention 破壊的
     */
    void put(point_t p, int n);
    /**
     * @brief どこにも石が置かれていないか
     */
    bool is_new() const;
};

/**
 * @brief 役割: 原点と大きさと回転と反転の概念の吸収
 */
class block {
public:
    static constexpr int N = block_size;
private:
    bool m_cell[2][4][N][N]; // m_cells?
    point_t m_offset[2][4];
    point_t m_size[2][4];
    int m_area;
    bool m_duplicated[2][4]; // true => you should ignore it
    std::vector<point_t> m_stones[2][4];
public:
    block();
    block(block_t const & a);

public:
    point_t offset(flip_t f, rot_t r) const;
    point_t size(flip_t f, rot_t r) const;
    point_t offset(placement_t const & p) const;
    point_t size(placement_t const & p) const;
    int area() const;
    int w(flip_t f, rot_t r) const;
    int h(flip_t f, rot_t r) const;
    int w(placement_t const & p) const;
    int h(placement_t const & p) const;
    point_t local(flip_t f, rot_t r, point_t const & q) const;
    bool at(flip_t f, rot_t r, point_t p) const;
    point_t world(placement_t const & p, point_t const & q) const;
    bool is_duplicated(flip_t f, rot_t r) const;
    std::vector<point_t> const & stones(flip_t f, rot_t r) const;
};
