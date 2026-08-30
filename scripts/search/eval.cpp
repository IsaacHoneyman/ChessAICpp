#include "eval.hpp"

int evaluate(const Board& board) {
    return board.toMove == WHITE ? board.pstScore : -board.pstScore;
}
 
int evaluateFromScratch(const Board& board) {
    int s = 0;
    for (int sq = 0; sq < BOARD_SIZE; ++sq) s += PST[board.at(sq)][sq];
    return s;
}