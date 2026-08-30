#pragma once
#include "types.hpp"
#include <array>
#include <cstdint>

namespace detail {

struct Zobrist {
    std::array<std::array<uint64_t, 64>, 15> pieces;
    std::array<uint64_t, 16> castling;
    std::array<uint64_t, 8> enPassant;
    uint64_t toMove;
};

constexpr uint64_t nextRandom(uint64_t &state) { // splitmix64
    state += 0x9E3779B97F4A7C15ULL;
    uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);    
}

constexpr Zobrist generateZobrist() {
    Zobrist keys{};

    uint64_t state{0ULL}; // arbitry starting seed

    keys.toMove = nextRandom(state);

    for (const Piece p : PIECE_LOOKUP) {
        for (int square = 0; square < 64; ++square) {
            keys.pieces[p][square] = nextRandom(state);
        }
    }

    std::array<uint64_t, 4> castlingRights{};
    for (int i = 0; i < 4; ++i) castlingRights[i] = nextRandom(state);
    for (int i = 0; i < 16; ++i) {
        uint64_t comp{0ULL};
        if (i & 1) comp ^= castlingRights[0];
        if (i & 2) comp ^= castlingRights[1];
        if (i & 4) comp ^= castlingRights[2];
        if (i & 8) comp ^= castlingRights[3];

        keys.castling[i] = comp;
    }

    for (int i = 0; i < 8; i++) keys.enPassant[i] = nextRandom(state);

    return keys;
}

} // namespace detail

inline constexpr detail::Zobrist ZOBRIST_KEYS = detail::generateZobrist();

// used when grabbing from FEN, so not in hotpath and hence not inlined
struct Board;
uint64_t getZobrist(const Board &board);