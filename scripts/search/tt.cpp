#include "tt.hpp"
#include <bit>

void TranspositionTable::resize(size_t megabytes) {
    size_t n = (megabytes * 1024 * 1024) / sizeof(TTEntry);
    n = std::bit_floor(n < 1024 ? size_t(1024) : n);  // power of two for masking
    table.assign(n, TTEntry{});
    mask = n - 1;
}

void TranspositionTable::clear() { table.assign(table.size(), TTEntry{}); }

bool TranspositionTable::probe(uint64_t key, int depth, int ply, int alpha,
                               int beta, int& score, Move& ttMove) const {
    const TTEntry& e = table[key & mask];
    if (e.key != key || e.bound == Bound::NONE) return false;

    ttMove = e.move;  // ordering

    if (e.depth < depth) return false;

    const int s = fromTT(e.score, ply);
    switch (e.bound) {
    case Bound::EXACT: score = s; return true;
    case Bound::LOWER: if (s >= beta)  { score = s; return true; } break;
    case Bound::UPPER: if (s <= alpha) { score = s; return true; } break;
    default: break;
    }
    return false;
}

void TranspositionTable::store(uint64_t key, int depth, int ply, int score, Bound bound, Move move) {
    TTEntry& e = table[key & mask];

    if (e.key == key && e.depth > depth && bound != Bound::EXACT) return;
    if (move == NO_MOVE && e.key == key) move = e.move; // keep old move

    e.key = key;
    e.score = int16_t(toTT(score, ply));
    e.move = move;
    e.depth = uint8_t(depth);
    e.bound = bound;
}