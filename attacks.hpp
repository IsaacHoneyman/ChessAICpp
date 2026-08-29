#pragma once
#include "bitboard.hpp"
#include "types.hpp"
#include "board.hpp"
#include <array>
#include <cstdint>

namespace detail {
constexpr int KNIGHT_DELTAS[8][2] = {{1, 2},   {2, 1},   {2, -1}, {1, -2},
                                     {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};

constexpr int KING_DELTAS[8][2] = {{0, 1},  {1, 1},   {1, 0},  {1, -1},
                                   {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}};

constexpr int W_PAWN_DELTAS[2][2] = {{-1, 1}, {1, 1}};
constexpr int B_PAWN_DELTAS[2][2] = {{-1, -1}, {1, -1}};

constexpr int ROOK_DELTAS[4][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
constexpr int BISHOP_DELTAS[4][2] = {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}};

template <size_t N>
constexpr std::array<uint64_t, BOARD_SIZE> generateStepAttacks(const int (&deltas)[N][2]) {
    std::array<uint64_t, BOARD_SIZE> out{};

    for (int i = 0; i < BOARD_SIZE; i++) { // square number
        int file = fileOf(i), rank = rankOf(i);
        for (const auto &d : deltas) {
            int f = file + d[0], r = rank + d[1];
            if (onBoard(f, r))
                out[i] |= squareBB(squareOf(f, r));
        }
    }

    return out;
}

constexpr uint64_t slidingAttacks(int sq, uint64_t occ, const int (&deltas)[4][2]) {
    uint64_t out = 0;
    for (const auto &d : deltas) {
        int f = fileOf(sq), r = rankOf(sq);
        while (true) {
            f += d[0];
            r += d[1];
            if (!onBoard(f, r))
                break;
            int t = squareOf(f, r);
            out |= squareBB(t);
            if (occ & squareBB(t))
                break;
        }
    }
    return out;
}

// BETWEEN[a][b] = squares strictly between a and b when they share a rank,
// file or diagonal; 0 otherwise. Used to find the blockers on a pin ray.
inline std::array<std::array<uint64_t, BOARD_SIZE>, BOARD_SIZE> makeBetween() {
    std::array<std::array<uint64_t, BOARD_SIZE>, BOARD_SIZE> out{};
    for (int a = 0; a < BOARD_SIZE; ++a) {
        for (int b = 0; b < BOARD_SIZE; ++b) {
            if (a == b) continue;
            if (detail::slidingAttacks(a, 0, detail::ROOK_DELTAS) & squareBB(b))
                out[a][b] = detail::slidingAttacks(a, squareBB(b), detail::ROOK_DELTAS)
                          & detail::slidingAttacks(b, squareBB(a), detail::ROOK_DELTAS);
            else if (detail::slidingAttacks(a, 0, detail::BISHOP_DELTAS) & squareBB(b))
                out[a][b] = detail::slidingAttacks(a, squareBB(b), detail::BISHOP_DELTAS)
                          & detail::slidingAttacks(b, squareBB(a), detail::BISHOP_DELTAS);
        }
    }
    return out;
}

} // namespace detail

namespace magic {
struct Magic {
    uint64_t mask; // squares that can block piece
    uint64_t multiplier; // get all important bits together
    uint32_t offset; // position in table 
    uint8_t shift; // shift pattern to bottom

    constexpr uint32_t index(uint64_t occupancy) const {
        return uint32_t(((occupancy & mask) * multiplier) >> shift);
    }
};

// selects bits from mask
constexpr uint64_t subsetOf(uint32_t index, uint64_t mask) {
    uint64_t subset = 0;
    for (int bit = 0; mask; ++bit) {
        int square = std::countr_zero(mask);
        mask &= mask - 1;
        if (index & (1U << bit))
            subset |= squareBB(square);
    }
    return subset;
}

constexpr std::array<Magic, BOARD_SIZE> ROOK_MAGICS = {{
    {0x101010101017eULL, 0x1080004008801020ULL, 0, 52}, {0x202020202027cULL, 0x840092002c03000ULL, 4096, 53},
    {0x404040404047aULL, 0x1900200010400900ULL, 6144, 53}, {0x8080808080876ULL, 0x880100008000480ULL, 8192, 53},
    {0x1010101010106eULL, 0x4200100420080200ULL, 10240, 53}, {0x2020202020205eULL, 0x8100020100080400ULL, 12288, 53},
    {0x4040404040403eULL, 0x200040110886200ULL, 14336, 53}, {0x8080808080807eULL, 0x200008040220411ULL, 16384, 52},
    {0x1010101017e00ULL, 0x404800084400220ULL, 20480, 53}, {0x2020202027c00ULL, 0x401000402000ULL, 22528, 54},
    {0x4040404047a00ULL, 0x86001081220440ULL, 23552, 54}, {0x8080808087600ULL, 0x408800800100280ULL, 24576, 54},
    {0x10101010106e00ULL, 0xa001201040820ULL, 25600, 54}, {0x20202020205e00ULL, 0x8848800200840080ULL, 26624, 54},
    {0x40404040403e00ULL, 0x4001000100040200ULL, 27648, 54}, {0x80808080807e00ULL, 0x442000102105084ULL, 28672, 53},
    {0x10101017e0100ULL, 0x9080010020804100ULL, 30720, 53}, {0x20202027c0200ULL, 0x40404000201009ULL, 32768, 54},
    {0x40404047a0400ULL, 0x808010002009ULL, 33792, 54}, {0x8080808760800ULL, 0x2200090021d00100ULL, 34816, 54},
    {0x101010106e1000ULL, 0x8008008040080ULL, 35840, 54}, {0x202020205e2000ULL, 0x4004002010040ULL, 36864, 54},
    {0x404040403e4000ULL, 0x11040008015042ULL, 37888, 54}, {0x808080807e8000ULL, 0xa0001768104ULL, 38912, 53},
    {0x101017e010100ULL, 0x800080204009ULL, 40960, 53}, {0x202027c020200ULL, 0x2010004140002001ULL, 43008, 54},
    {0x404047a040400ULL, 0x9800200280100080ULL, 44032, 54}, {0x8080876080800ULL, 0x1000100080080080ULL, 45056, 54},
    {0x1010106e101000ULL, 0x442000a00049020ULL, 46080, 54}, {0x2020205e202000ULL, 0x2100040080020080ULL, 47104, 54},
    {0x4040403e404000ULL, 0x800120400900148ULL, 48128, 54}, {0x8080807e808000ULL, 0x10040a00128541ULL, 49152, 53},
    {0x1017e01010100ULL, 0x2800804000800030ULL, 51200, 53}, {0x2027c02020200ULL, 0x1010002000400041ULL, 53248, 54},
    {0x4047a04040400ULL, 0x4000200011004100ULL, 54272, 54}, {0x8087608080800ULL, 0x610008410800800ULL, 55296, 54},
    {0x10106e10101000ULL, 0x400802402800800ULL, 56320, 54}, {0x20205e20202000ULL, 0xc100020080800400ULL, 57344, 54},
    {0x40403e40404000ULL, 0x2000802000401ULL, 58368, 54}, {0x80807e80808000ULL, 0x182085882000401ULL, 59392, 53},
    {0x17e0101010100ULL, 0x220204000808000ULL, 61440, 53}, {0x27c0202020200ULL, 0x2860100040024022ULL, 63488, 54},
    {0x47a0404040400ULL, 0x1002004110040ULL, 64512, 54}, {0x8760808080800ULL, 0x99101042000a0020ULL, 65536, 54},
    {0x106e1010101000ULL, 0x4080004008080ULL, 66560, 54}, {0x205e2020202000ULL, 0x10040002008080ULL, 67584, 54},
    {0x403e4040404000ULL, 0x2012004881020004ULL, 68608, 54}, {0x807e8080808000ULL, 0x8300842444820011ULL, 69632, 53},
    {0x7e010101010100ULL, 0x88403882010200ULL, 71680, 53}, {0x7c020202020200ULL, 0x820400080210100ULL, 73728, 54},
    {0x7a040404040400ULL, 0x110910040a00300ULL, 74752, 54}, {0x76080808080800ULL, 0x801100280080480ULL, 75776, 54},
    {0x6e101010101000ULL, 0x242009008200600ULL, 76800, 54}, {0x5e202020202000ULL, 0x1002000489500200ULL, 77824, 54},
    {0x3e404040404000ULL, 0x40800200010080ULL, 78848, 54}, {0x7e808080808000ULL, 0x91800041000080ULL, 79872, 53},
    {0x7e01010101010100ULL, 0x209300488001ULL, 81920, 52}, {0x7c02020202020200ULL, 0x4c1002414824001ULL, 86016, 53},
    {0x7a04040404040400ULL, 0x20020000b001041ULL, 88064, 53}, {0x7608080808080800ULL, 0x7000100004200901ULL, 90112, 53},
    {0x6e10101010101000ULL, 0x8002002004100802ULL, 92160, 53}, {0x5e20202020202000ULL, 0x30010002084c0007ULL, 94208, 53},
    {0x3e40404040404000ULL, 0x888221800813004ULL, 96256, 53}, {0x7e80808080808000ULL, 0x4000002840840112ULL, 98304, 52}
}};

constexpr std::array<Magic, BOARD_SIZE> BISHOP_MAGICS = {{
    {0x40201008040200ULL, 0x10102002004a1420ULL, 0, 58}, {0x402010080400ULL, 0x8020040400584008ULL, 64, 59},
    {0x4020100a00ULL, 0x10510800811201c8ULL, 96, 59}, {0x40221400ULL, 0x5204042080000088ULL, 128, 59},
    {0x2442800ULL, 0x2204106880000002ULL, 160, 59}, {0x204085000ULL, 0x1401042004000000ULL, 192, 59},
    {0x20408102000ULL, 0x400880410042004ULL, 224, 59}, {0x2040810204000ULL, 0x28208200a02020ULL, 256, 58},
    {0x20100804020000ULL, 0x1500241990010e00ULL, 320, 59}, {0x40201008040000ULL, 0x8001200182020a40ULL, 352, 59},
    {0x4020100a0000ULL, 0x40004101030b0000ULL, 384, 59}, {0x4022140000ULL, 0x8002041042000100ULL, 416, 59},
    {0x244280000ULL, 0x4010011041020038ULL, 448, 59}, {0x20408500000ULL, 0x10421044000ULL, 480, 59},
    {0x2040810200000ULL, 0x1500210808020a00ULL, 512, 59}, {0x4081020400000ULL, 0x8000088400880520ULL, 544, 59},
    {0x10080402000200ULL, 0x405004010040100ULL, 576, 59}, {0x20100804000400ULL, 0x1005823210040108ULL, 608, 59},
    {0x4020100a000a00ULL, 0x2708008102040011ULL, 640, 57}, {0x402214001400ULL, 0x4048200404009100ULL, 768, 57},
    {0x24428002800ULL, 0x18104101400024ULL, 896, 57}, {0x2040850005000ULL, 0x3000601190101ULL, 1024, 57},
    {0x4081020002000ULL, 0x8004803108491000ULL, 1152, 59}, {0x8102040004000ULL, 0x8014241200820800ULL, 1184, 59},
    {0x8040200020400ULL, 0x6e080100c3040ULL, 1216, 59}, {0x10080400040800ULL, 0x501044a11041800ULL, 1248, 59},
    {0x20100a000a1000ULL, 0x9020300008004045ULL, 1280, 57}, {0x40221400142200ULL, 0x894080000220040ULL, 1408, 55},
    {0x2442800284400ULL, 0x1001010083104000ULL, 1920, 55}, {0x4085000500800ULL, 0x5004030040900080ULL, 2432, 57},
    {0x8102000201000ULL, 0x400422c012400ULL, 2560, 59}, {0x10204000402000ULL, 0x2128698404812ULL, 2592, 59},
    {0x4020002040800ULL, 0x1010108404900440ULL, 2624, 59}, {0x8040004081000ULL, 0x928021182084100ULL, 2656, 59},
    {0x100a000a102000ULL, 0x2006080409020024ULL, 2688, 57}, {0x22140014224000ULL, 0x1010202020180080ULL, 2816, 55},
    {0x44280028440200ULL, 0xa010008200202200ULL, 3328, 55}, {0x8500050080400ULL, 0x2098015100019004ULL, 3840, 57},
    {0x10200020100800ULL, 0x2041440810811ULL, 3968, 59}, {0x20400040201000ULL, 0x802a02020000b098ULL, 4000, 59},
    {0x2000204081000ULL, 0x9015090004060ULL, 4032, 59}, {0x4000408102000ULL, 0x4000821082081001ULL, 4064, 59},
    {0xa000a10204000ULL, 0x100210040420800ULL, 4096, 57}, {0x14001422400000ULL, 0x800004010488a00ULL, 4224, 57},
    {0x28002844020000ULL, 0x2000081104004040ULL, 4352, 57}, {0x50005008040200ULL, 0x4c8e029015000082ULL, 4480, 57},
    {0x20002010080400ULL, 0x420340322224842ULL, 4608, 59}, {0x40004020100800ULL, 0x1298260043400210ULL, 4640, 59},
    {0x20408102000ULL, 0x822802400008ULL, 4672, 59}, {0x40810204000ULL, 0x8a0101600000ULL, 4704, 59},
    {0xa1020400000ULL, 0x3040003412080021ULL, 4736, 59}, {0x142240000000ULL, 0x3040290220884800ULL, 4768, 59},
    {0x284402000000ULL, 0x4a1500401041004aULL, 4800, 59}, {0x500804020000ULL, 0x8010200282020781ULL, 4832, 59},
    {0x201008040200ULL, 0x20203142209091ULL, 4864, 59}, {0x402010080400ULL, 0x70300600902110ULL, 4896, 59},
    {0x2040810204000ULL, 0x40808800b62048ULL, 4928, 58}, {0x4081020400000ULL, 0x810400c44420ULL, 4992, 59},
    {0xa102040000000ULL, 0x80400440c0441ULL, 5024, 59}, {0x14224000000000ULL, 0x8340080020840411ULL, 5056, 59},
    {0x28440200000000ULL, 0x104208200ULL, 5088, 59}, {0x50080402000000ULL, 0x800810d00080ULL, 5120, 59},
    {0x20100804020000ULL, 0x400530411080200ULL, 5152, 59}, {0x40201008040200ULL, 0x4040702400932244ULL, 5184, 58}
}};

template <size_t TableSize>
std::array<uint64_t, TableSize> makeTable(
    const std::array<Magic, BOARD_SIZE>& magics, const int (&deltas)[4][2]) {
    std::array<uint64_t, TableSize> table{};
    for (int square = 0; square < BOARD_SIZE; ++square) {
        const Magic& m = magics[square];
        uint32_t count = 1U << popCount(m.mask);
        for (uint32_t subset = 0; subset < count; ++subset) {
            uint64_t occupancy = subsetOf(subset, m.mask);
            table[m.offset + m.index(occupancy)] =
                detail::slidingAttacks(square, occupancy, deltas);
        }
    }
    return table;
}

inline const auto ROOK_TABLE = makeTable<102400>(ROOK_MAGICS, detail::ROOK_DELTAS);
inline const auto BISHOP_TABLE = makeTable<5248>(BISHOP_MAGICS, detail::BISHOP_DELTAS);

} // namespace magic

// --- Interface ---

constexpr auto KNIGHT_ATTACKS = detail::generateStepAttacks(detail::KNIGHT_DELTAS);
constexpr auto KING_ATTACKS = detail::generateStepAttacks(detail::KING_DELTAS);
constexpr std::array<std::array<uint64_t, BOARD_SIZE>, COLOUR_SIZE> PAWN_ATTACKS = {
    detail::generateStepAttacks(detail::W_PAWN_DELTAS),
    detail::generateStepAttacks(detail::B_PAWN_DELTAS)};

inline uint64_t rookAttacks(int square, uint64_t occupancy) {
    const auto& magic = magic::ROOK_MAGICS[square];
    return magic::ROOK_TABLE[magic.offset + magic.index(occupancy)];
}

inline uint64_t bishopAttacks(int square, uint64_t occupancy) {
    const auto& magic = magic::BISHOP_MAGICS[square];
    return magic::BISHOP_TABLE[magic.offset + magic.index(occupancy)];
}

inline uint64_t queenAttacks(int square, uint64_t occupancy) {
    return rookAttacks(square, occupancy) | bishopAttacks(square, occupancy);
}

inline const auto BETWEEN = detail::makeBetween();

inline bool isAttacked(const Board& board, int square, PieceColour by, uint64_t occupancy) {
    return board.byColour[by] & (
    (board.byType[KNIGHT] & KNIGHT_ATTACKS[square]) |
    (board.byType[KING]   & KING_ATTACKS[square]  ) |
    ((board.byType[BISHOP] | board.byType[QUEEN]) & bishopAttacks(square, occupancy)) |
    ((board.byType[ROOK] | board.byType[QUEEN]) & rookAttacks(square, occupancy)) |
    (board.byType[PAWN] & PAWN_ATTACKS[other(by)][square])
    );
}

inline bool isAttacked(const Board& board, int square, PieceColour by) {
    return isAttacked(board, square, by, board.occupied());
}
