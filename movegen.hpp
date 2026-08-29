#pragma once
#include "board.hpp"
#include "history.hpp"
#include "types.hpp"
#include <cassert>

struct MoveList {
    Move moves[256]; // can never have more than 218 legal moves in a position
    int count = 0;
    void push_back(Move m) { assert(count < 256); moves[count++] = m; }
    Move* begin() { return moves; }
    Move* end() { return moves + count; }
    int size() const { return count; }
};

enum class GameState { ONGOING, CHECKMATE, STALEMATE, FIFTYMOVE, INSUFFICIENT, REPETITION };

void generatePseudoLegal(const Board& board, MoveList& moves);
void generateLegal(Board& board, MoveList& moves);

bool inCheck(const Board& board, PieceColour mover);
GameState getState(const Board& board, const MoveList& moves, const PositionHistory& history);
