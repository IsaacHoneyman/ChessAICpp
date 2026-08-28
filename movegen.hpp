#pragma once
#include "board.hpp"
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

void generateLegal(const Board& board, MoveList& moves);

