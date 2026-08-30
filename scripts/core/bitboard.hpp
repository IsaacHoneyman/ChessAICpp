#pragma once
#include "types.hpp"
#include <cstdint>
#include <bit>
#include <cassert>
#include <iostream>

constexpr uint64_t squareBB(int square) { return 1ULL << square; }
constexpr int fileOf(int square) { return square & 7; } // x
constexpr int rankOf(int square) { return square >> 3; } // y
constexpr int squareOf(int file, int rank) { return rank * 8 + file; }
constexpr bool onBoard(int square) { return square >= 0 && square < BOARD_SIZE; }
constexpr bool onBoard(int file, int rank) { return rank >= 0 && rank < RANK_COUNT && file >= 0 && file < FILE_COUNT; }

constexpr int popCount(uint64_t bb) { return std::popcount(bb); }
constexpr int lsb(uint64_t bb) { assert(bb); return std::countr_zero(bb); }

inline int popLsb(uint64_t& bb) {
    assert(bb);
    int sq = std::countr_zero(bb);
    bb &= bb - 1;
    return sq;
}

constexpr uint64_t RANK_1 = 0x00000000000000FFULL; // y
constexpr uint64_t RANK_2 = RANK_1 << 8;
constexpr uint64_t RANK_3 = RANK_1 << 16;
constexpr uint64_t RANK_4 = RANK_1 << 24;
constexpr uint64_t RANK_5 = RANK_1 << 32;
constexpr uint64_t RANK_6 = RANK_1 << 40;
constexpr uint64_t RANK_7 = RANK_1 << 48;
constexpr uint64_t RANK_8 = RANK_1 << 56;

constexpr uint64_t FILE_A = 0x0101010101010101ULL; // x
constexpr uint64_t FILE_B = FILE_A << 1;
constexpr uint64_t FILE_C = FILE_A << 2;
constexpr uint64_t FILE_D = FILE_A << 3;
constexpr uint64_t FILE_E = FILE_A << 4;
constexpr uint64_t FILE_F = FILE_A << 5;
constexpr uint64_t FILE_G = FILE_A << 6;
constexpr uint64_t FILE_H = FILE_A << 7;

constexpr uint64_t LIGHT_SQUARES = 0x55AA55AA55AA55AAULL;
constexpr uint64_t DARK_SQUARES  = ~LIGHT_SQUARES;

inline void printBitboard(uint64_t bb) {
    for (int rank = 7; rank >= 0; --rank) {
        std::cerr << (rank + 1) << "  ";
        for (int file = 0; file < 8; ++file)
            std::cerr << ((bb & squareBB(squareOf(file, rank))) ? '1' : '.') << ' ';
        std::cerr << '\n';
    }
    std::cerr << "\n   a b c d e f g h\n"
              << "   popcount: " << popCount(bb) << "\n\n";
}