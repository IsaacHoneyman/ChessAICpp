#pragma once
#include "game.hpp"
#include "search.hpp"

struct Bot {
    virtual ~Bot() = default;
    virtual SearchResult pick(Game& g, const MoveList& legal) = 0;
    virtual std::string name() const = 0;
    virtual void newGame() {}   // clear TT, killers, history tables
};