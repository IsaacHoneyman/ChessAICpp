#pragma once
#include "game.hpp"
#include "search.hpp"
#include <optional>

struct Bot {
    virtual ~Bot() = default;
    virtual SearchResult pick(Game& g, const MoveList& legal, std::optional<SearchLimits> limits) = 0;
    virtual std::string name() const = 0;
    virtual void newGame() {}   // clear TT, killers, history tables
};