#pragma once
#include "board.hpp"
#include "types.hpp"

namespace detail {

inline constexpr int VALUE[TYPE_SIZE] = {0, 100, 320, 330, 500, 900, 0};

constexpr int BONUS[TYPE_SIZE][64] = {
    {},  // NO_TYPE
 
    // PAWN -- advance, take the centre, don't push the pawns in front of a
    // castled king. Rank 1 and 8 are zero since a pawn can never stand there.
    {  0,  0,  0,  0,  0,  0,  0,  0,
      50, 50, 50, 50, 50, 50, 50, 50,
      10, 10, 20, 30, 30, 20, 10, 10,
       5,  5, 10, 25, 25, 10,  5,  5,
       0,  0,  0, 20, 20,  0,  0,  0,
       5, -5,-10,  0,  0,-10, -5,  5,
       5, 10, 10,-20,-20, 10, 10,  5,
       0,  0,  0,  0,  0,  0,  0,  0 },
 
    // KNIGHT -- centralise; the rim is dim.
    {-50,-40,-30,-30,-30,-30,-40,-50,
     -40,-20,  0,  0,  0,  0,-20,-40,
     -30,  0, 10, 15, 15, 10,  0,-30,
     -30,  5, 15, 20, 20, 15,  5,-30,
     -30,  0, 15, 20, 20, 15,  0,-30,
     -30,  5, 10, 15, 15, 10,  5,-30,
     -40,-20,  0,  5,  5,  0,-20,-40,
     -50,-40,-30,-30,-30,-30,-40,-50 },
 
    // BISHOP -- long diagonals, off the edges.
    {-20,-10,-10,-10,-10,-10,-10,-20,
     -10,  0,  0,  0,  0,  0,  0,-10,
     -10,  0,  5, 10, 10,  5,  0,-10,
     -10,  5,  5, 10, 10,  5,  5,-10,
     -10,  0, 10, 10, 10, 10,  0,-10,
     -10, 10, 10, 10, 10, 10, 10,-10,
     -10,  5,  0,  0,  0,  0,  5,-10,
     -20,-10,-10,-10,-10,-10,-10,-20 },
 
    // ROOK -- the seventh rank, and the centre files at home.
    {  0,  0,  0,  0,  0,  0,  0,  0,
       5, 10, 10, 10, 10, 10, 10,  5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
       0,  0,  0,  5,  5,  0,  0,  0 },
 
    // QUEEN -- mild centralisation, discourage early sorties.
    {-20,-10,-10, -5, -5,-10,-10,-20,
     -10,  0,  0,  0,  0,  0,  0,-10,
     -10,  0,  5,  5,  5,  5,  0,-10,
      -5,  0,  5,  5,  5,  5,  0, -5,
       0,  0,  5,  5,  5,  5,  0, -5,
     -10,  5,  5,  5,  5,  5,  0,-10,
     -10,  0,  5,  0,  0,  0,  0,-10,
     -20,-10,-10, -5, -5,-10,-10,-20 },
 
    // KING -- middlegame only. Castle and stay behind the pawns. This table is
    // actively wrong in an endgame, where the king belongs in the centre; that
    // needs a second table and phase interpolation, which is a later job.
    {-30,-40,-40,-50,-50,-40,-40,-30,
     -30,-40,-40,-50,-50,-40,-40,-30,
     -30,-40,-40,-50,-50,-40,-40,-30,
     -30,-40,-40,-50,-50,-40,-40,-30,
     -20,-30,-30,-40,-40,-30,-30,-20,
     -10,-20,-20,-20,-20,-20,-20,-10,
      20, 20,  0,  0,  0,  0, 20, 20,
      20, 30, 10,  0,  0, 10, 30, 20 },
};

constexpr std::array<std::array<int, 64>, 15> makePST() {
    std::array<std::array<int, 64>, 15> t{};
    for (const PieceType pt : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING}) {
        for (int sq = 0; sq < BOARD_SIZE; ++sq) {
            t[makePiece(WHITE, pt)][sq] =   VALUE[pt] + BONUS[pt][sq ^ 56];
            t[makePiece(BLACK, pt)][sq] = -(VALUE[pt] + BONUS[pt][sq]);
        }
    }
    return t;
}

} // namespace detail

inline constexpr auto PST = detail::makePST();

int evaluateFromScratch(const Board& board);
int evaluate(const Board& board);