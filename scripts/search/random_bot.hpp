#pragma once
#include "bot.hpp"
#include <random>

struct RandomBot : Bot {
    explicit RandomBot(uint32_t seed = std::random_device{}()) : rng(seed) {}

    SearchResult pick(Game&, const MoveList& legal) override {
        SearchResult r;
        r.move = legal.moves[rng() % legal.size()];
        r.nodes = uint64_t(legal.size());
        return r;
    }

    std::string name() const override { return "RandomBot"; }

private:
    std::mt19937 rng;
};