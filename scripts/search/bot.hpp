#pragma once
#include "game.hpp"
#include "types.hpp"

struct SearchResult {
    Move move  = NO_MOVE;
    int  score = 0;        // centipawns, positive = good for side to move
    int  depth = 0;
    uint64_t nodes = 0;
};

struct Bot {
    virtual ~Bot() = default;
    virtual SearchResult pick(Game& g, const MoveList& legal) = 0;
    virtual std::string name() const = 0;
    virtual void newGame() {}   // clear TT, killers, history tables
};