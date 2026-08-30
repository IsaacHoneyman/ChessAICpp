#pragma once
#include "game.hpp"
#include "movegen.hpp"
#include "types.hpp"
#include <chrono>
#include <cstdint>

inline constexpr int MATE = 30000; // mate given this value (treat kinda like infinity)
inline constexpr int MATE_THRESHOLD = MATE - MAX_SEARCH_PLY;
inline constexpr int INF = 32000;

constexpr bool isMateScore(int s) { return s > MATE_THRESHOLD || s < -MATE_THRESHOLD; }

struct SearchResult {
    Move move  = NO_MOVE;
    int  score = 0;        // centipawns, positive = good for side to move
    int  depth = 0;
    uint64_t nodes = 0;
};

struct SearchLimits {
    int maxDepth = MAX_SEARCH_PLY;
    int64_t softMs = 0; // dont start another iteration
    int64_t hardMs = 0; // must abort midway through iteration
};

struct Searcher {
    SearchResult search(Game& g, const MoveList& root, SearchLimits limits);
    SearchResult search(Game& g, const MoveList& root, int depth) {
        return search(g, root, {depth, 0, 0});
    }

    void clear() { nodes = 0; }

private:
    using Clock = std::chrono::steady_clock;

    SearchResult searchRoot(Game& g, int depth);
    int negamax(Game&, int depth, int ply, int alpha, int beta);
    int quiesce(Game&, int alpha, int beta, int ply, int qply);

    int64_t msSince(Clock::time_point t) const;
    bool outOfTime(); // hard limit, 2048

    uint64_t nodes = 0;
    bool stopped = false;
    Clock::time_point start{};
    SearchLimits limits{};

    MoveList rootMoves{};
    Move prevBest = NO_MOVE;
};