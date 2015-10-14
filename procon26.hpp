/**
 * @file procon26.hpp
 * @author Kimiyuki Onaka
 * @brief 効率良い探索のために必要な情報を添えた、盤と石のclass
 */
#pragma once
#include <vector>
#include <bitset>
#include <set>
#include <cassert>
#include <cstdint>
#include "common.hpp"

/**
 * CAUTION: there are *6* coordinates. (YABAI)
 * + board world
 *  + board local
 *   + block + placement world
 *    + block + placement local
 * + block world
 *  + block local
 */

/**
 * やばいので整理します
 */

/**
 * 以下のようにblockがあったときに、
 * 00000000
 * 00111110
 * 00001000
 * 00001100
 * 00000000
 * 00000000
 * 00000000
 * 00000000
 * (0,0) を 原点とするのが block world 座標系
 * (2,1) を 原点とするのが block local 座標系
 */

/**
 * 以下のようにboardに上で見たblockが配置されているとき
 * +012345678901234567890
 * 011111111111111111111111111111111111
 * 111111111111111111111111111111111111
 * 211100000000000000000000000001111111
 * 311100000000000000000000000001111111
 * 411100000000000000000000000001111111
 * 511100000010000000000000000001111111
 * 611100000010100000000000000001111111
 * 711100000011100000000000000001111111
 * 811100000010000000000000000001111111
 * 911100000010000000000000000001111111
 * 011100000000000000000000000001111111
 * 111100000000000000000000000001111111
 * 211100000000000000000000000001111111
 * 311100000000000000000000000001111111
 * 411100000000000000000000000001111111
 * 511111111111111111111111111111111111
 * 611111111111111111111111111111111111
 * (0,0) を 原点とするのが board world 座標系
 * (5,2) を 原点とするのが board local 座標系
 * (9,5) を 原点とするのが block + placement world 座標系
 * (8,4) を 原点とするのが block + placement local 座標系
 */

/**
 * やばいね
 */


class board;
class block;

/**
 * @brief 役割: 原点と大きさの概念と置いた石の情報の吸収
 * @attention MUTABLE
 * @note fieldって呼ぶべきだったかも
 */
class board {
public:
    static constexpr int N = board_size;
public:
    int m_cell[N][N];
    point_t m_offset; /// @brief 空白のbounding boxの
    point_t m_size;
    int m_area; /// @brief 空白の
    point_t m_stone_offset; /// @brief 既に置いてある石のbounding boxの
    point_t m_stone_size;
    int m_stone_area;
    int m_first_stone; /// @brief 自由な位置に置いた石のindex
    int m_skips[N][N]; /// @brief boyer-moore的なあれのためのそれ
    std::bitset<board_size*board_size> m_packed; /// 

public:
    board();
    explicit board(board_t const & a);

public:
    bool is_intersect(block const & blk, placement_t const & p) const;
    bool is_intersect(block const & blk, placement_t const & p, int *skip) const;
    bool is_puttable(block const & blk, placement_t const & p, int n, int *skip) const;
    /**
     * @attention 破壊的 無確認
     * @attention 後続してupdateを呼ぶこと
     * @param p in board world
     * @param value 0, 1 or 2+n
     */
    void put(point_t p, int value);
    void put(block const & blk, placement_t const & p, int value);
    /**
     * @brief areaやis_newを更新
     * @attention put後 必須
     */
    void update();
    /**
     * @brief offsetとsize等を更新
     * @attention skipの情報が更新されるため探索の前に呼ぶとよい
     */
    void shrink();
    /**
     * @attention 遅い
     */
    std::vector<board> split() const;
public:
    /**
     * @brief (0,0) of board local in board world
     */
    point_t offset() const;
    point_t size() const;
    point_t stone_offset() const;
    point_t stone_size() const;
    /**
     * @return
     *   - 0: 何も置かれていない
     *   - 1: 障害物
     *   - 2+n: n番目の石
     */
    int at_local(point_t p) const; // board local
    int at(point_t p) const; // board world
    int area() const;
    int stone_area() const;
    int w() const;
    int h() const;
    /**
     * @brief どこにも石が置かれていないか
     */
    bool is_new() const;
    /**
     * @param p in board world
     * @return どこまでずらしてもだめならはみ出る値が返る
     */
    int skip(point_t p) const;
    std::bitset<board_size*board_size> packed() const;
};

/**
 * @brief 役割: 原点と大きさと回転と反転の概念の吸収
 * @attention immutable
 */
class block {
public:
    static constexpr int N = block_size;
public:
    bool m_cell[2][4][N][N]; // m_cells?
    point_t m_offset[2][4];
    point_t m_size[2][4];
    int m_area;
    int m_circumference;
    bool m_duplicated[2][4]; /// @brief 真なら無視すべき
    std::vector<point_t> m_stones[2][4];
    std::vector<int> m_skips[2][4];
    uint64_t m_signature; /// @brief 位置や向きによらず形のみから定まる値
public:
    block();
    explicit block(block_t const & a);

public:
    point_t offset(flip_t f, rot_t r) const;
    point_t size(flip_t f, rot_t r) const;
    point_t offset(placement_t const & p) const;
    point_t size(placement_t const & p) const;
    int area() const;
    int circumference() const;
    int w(flip_t f, rot_t r) const;
    int h(flip_t f, rot_t r) const;
    int w(placement_t const & p) const;
    int h(placement_t const & p) const;
    /**
     * @param p in block world
     */
    bool at(flip_t f, rot_t r, point_t p) const;
    /**
     * @param q in board world
     */
    bool at(placement_t const & p, point_t q) const;
    bool is_duplicated(flip_t f, rot_t r) const;
    /**
     * @return in block world
     */
    // std::vector<point_t> const & stones(flip_t f, rot_t r) const;
    inline std::vector<point_t> const & stones(flip_t f, rot_t r) const { return m_stones[f][r]; }
    /**
     * @return in board world
     * @attention 遅い
     */
    [[deprecated]]
    std::vector<point_t> stones(placement_t const & p) const;
    /**
     * @return その点で障害物と衝突した際、次にx方向にいくらずらせばよいか
     */
    std::vector<int> const & skips(flip_t f, rot_t r) const;
    uint64_t signature() const;
};

bool is_intersect(board const & brd, block const & blk, placement_t const & p);
bool is_intersect(board const & brd, block const & blk, placement_t const & p, int *skip);
bool is_puttable(board const & brd, block const & blk, placement_t const & p);
bool is_puttable(board const & brd, block const & blk, placement_t const & p, int *skip);
/**
 * @pre 置ける is_puttableが真を返す
 * @attention 何も確認しないことに注意
 * @param brd
 *   変更される
 * @param value
 *   n番目の石を置くと: 2+n
 *   戻す: 0
 */
void put_stone(board & brd, block const & blk, placement_t const & p, int value);

/**
 * @brief 試すべき石の置き方を列挙
 */
placement_t initial_placement(block const & blk, point_t const & lp);
bool next_placement(placement_t & p, block const & blk, point_t const & lp, point_t const & rp);

/**
 * @attention for debug
 */
std::ostream & operator << (std::ostream & out, board const & brd);


/**
 * @brief m_signatureの値の表
 */
/* 1 1
 * 2 11
 * 3 111
 *    1
 * 4 11
 * 5 1111
 *     1
 * 6 111
 *    1
 * 7 111
 *   11
 * 8 11
 */
const uint64_t block_signatures[] = {
    9223372036854775808ull,
    9259400833873739776ull,
    9259541571362095104ull,
    4665729213955833856ull,
    9259542121117908992ull,
    2368893403996880896ull,
    4665799582700011520ull,
    13889101250810609664ull,
};
