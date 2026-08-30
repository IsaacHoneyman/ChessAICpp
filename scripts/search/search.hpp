#pragma once
#include "game.hpp"
#include "movegen.hpp"
#include "types.hpp"

inline constexpr int MATE = 30000; // mate given this value (treat kinda like infinity)
inline constexpr int MATE_THRESHOLD = MATE - MAX_SEARCH_PLY;
inline constexpr int INF = 32000;

struct SearchResult {
    Move move  = NO_MOVE;
    int  score = 0;        // centipawns, positive = good for side to move
    int  depth = 0;
    uint64_t nodes = 0;
};

struct Searcher {
    SearchResult search(Game& g, const MoveList& root, int depth);
    void clear();

private:
    int negamax(Game&, int depth, int ply, int alpha, int beta);
    int quiesce(Game&, int alpha, int beta, int ply, int qply);

    uint64_t nodes = 0;
};