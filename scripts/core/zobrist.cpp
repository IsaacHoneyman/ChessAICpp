#include "bitboard.hpp"
#include "types.hpp"
#include "board.hpp"
#include "zobrist.hpp"
#include <cstdint>

uint64_t getZobrist(const Board &board) {
    uint64_t hash = 0ULL;

    for (int i = 0; i < BOARD_SIZE; ++i) 
        hash ^= ZOBRIST_KEYS.pieces[board.at(i)][i]; // okay as x ^= 0 = x

    hash ^= ZOBRIST_KEYS.castling[board.castling & 0x0F];
    
    if (board.epSquare != NO_SQUARE) hash ^= ZOBRIST_KEYS.enPassant[fileOf(board.epSquare)];
    if (board.toMove == BLACK) hash ^= ZOBRIST_KEYS.toMove;

    return hash;
}