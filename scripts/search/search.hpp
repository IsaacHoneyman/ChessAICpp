#pragma once
#include "game.hpp"
#include "movegen.hpp"
#include "types.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>


struct SearchResult {
    Move move  = NO_MOVE;
    int  score = 0;        // centipawns, positive = good for side to move
    int  depth = 0;
    int  seldepth = 0;     // deepest ply reached, quiescence included
    uint64_t nodes = 0;
    uint64_t qnodes = 0;   // subset of nodes spent in quiescence
    uint64_t cutoffs = 0;      // beta cutoffs taken
    uint64_t firstCutoffs = 0; // of which fired on the first move tried
    int64_t ms = 0;        // elapsed since the search began
};

struct SearchLimits {
    int maxDepth = MAX_SEARCH_PLY;
    std::optional<int64_t> softMs;   
    std::optional<int64_t> hardMs;

    std::function<void(const SearchResult&)> onIteration{};
};

struct Searcher {
    SearchResult search(Game& g, const MoveList& root, SearchLimits limits);
    SearchResult search(Game& g, const MoveList& root, int depth) {
        return search(g, root, {depth, std::nullopt, std::nullopt, {}});
    }

    void clear() { nodes = qnodes = cutoffs = firstCutoffs = 0; seldepth = 0; }

private:
    using Clock = std::chrono::steady_clock;

    SearchResult searchRoot(Game& g, int depth);
    int negamax(Game&, int depth, int ply, int alpha, int beta);
    int quiesce(Game&, int alpha, int beta, int ply, int qply);

    int64_t msSince(Clock::time_point t) const;
    bool outOfTime(); // hard limit, 2048
    void fillStats(SearchResult& r); // copy the counters into a result

    uint64_t nodes = 0;
    uint64_t qnodes = 0;
    uint64_t cutoffs = 0;
    uint64_t firstCutoffs = 0;
    int seldepth = 0;
    bool stopped = false;
    Clock::time_point start{};
    SearchLimits limits{};

    MoveList rootMoves{};
    Move prevBest = NO_MOVE;
};