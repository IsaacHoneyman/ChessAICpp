#pragma once
#include "game.hpp"
#include "movegen.hpp"
#include "tt.hpp"
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

    void clear() { 
        nodes = qnodes = cutoffs = firstCutoffs = 0; 
        seldepth = 0;
        tt.clear(); 
        for (auto& row : history) for (auto& v : row) v = 0;
    }
    void setHashSize(size_t mb) { tt.resize(mb); }

private:
    using Clock = std::chrono::steady_clock;

    SearchResult searchRoot(Game& g, int depth);
    int negamax(Game&, int depth, int ply, int alpha, int beta, bool allowNmp = true);
    int quiesce(Game&, int alpha, int beta, int ply, int qply);
    void ageHistory();

    int64_t msSince(Clock::time_point t) const;
    bool outOfTime(); // hard limit, 2048
    void fillStats(SearchResult& r); // copy the counters into a result

    TranspositionTable tt;

    uint64_t nodes = 0;
    uint64_t qnodes = 0;
    uint64_t cutoffs = 0;
    uint64_t firstCutoffs = 0;
    int seldepth = 0;
    bool stopped = false;
    Clock::time_point start{};
    SearchLimits limits{};

    Move killers[MAX_SEARCH_PLY][2]{}; // two quite moves per ply that caused a beta cutoff, same refute 
    int history[15][64]{}; // moves like this recently have worked, aged overtime

    MoveList rootMoves{};
    Move prevBest = NO_MOVE;
};