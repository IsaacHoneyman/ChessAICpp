#include "eval.hpp"
#include "board.hpp"

int evaluate(const Board& board) {
    const int phase = std::min(board.gamePhase, TOTAL_PHASE);

    const int mgScore = board.pstScore.mg;
    const int egScore = board.pstScore.eg;
    const int score = (mgScore * phase + egScore * (TOTAL_PHASE - phase)) / TOTAL_PHASE;

    return board.toMove == WHITE ? score : -score;
}

int evaluateFromScratch(const Board& board) {
    Score s{};
    for (int sq = 0; sq < 64; ++sq) {
        s += PST[board.at(sq)][sq];
    }
    const int phase = std::min(board.gamePhase, TOTAL_PHASE);
    const int score = (s.mg * phase + s.eg * (TOTAL_PHASE - phase)) / TOTAL_PHASE;
    return board.toMove == WHITE ? score : -score;
}