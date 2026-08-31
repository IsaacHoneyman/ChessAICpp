#pragma once
#include "types.hpp"
#include <cstdint>
#include <vector>

enum class Bound : uint8_t {
    NONE = 0,
    EXACT,  // score is the true value: 
    LOWER,  // failed high: true value is >= score
    UPPER,  // failed low:  true value is <= score
};

struct TTEntry {
    uint64_t key = 0;  
    int16_t score = 0;
    Move move{};
    uint8_t depth = 0;
    Bound bound = Bound::NONE;
};
static_assert(sizeof(TTEntry) <= 16, "entry should stay small enough to pack");

struct TranspositionTable {
    explicit TranspositionTable(size_t megabytes = 64) { resize(megabytes); }

    void resize(size_t megabytes);
    void clear();

    bool probe(uint64_t key, int depth, int ply, int alpha, int beta,
               int& score, Move& ttMove) const;

    void store(uint64_t key, int depth, int ply, int score, Bound bound, Move move);

    size_t entries() const { return table.size(); }

private: // mates change relative to ply found
    static int toTT(int score, int ply) {
        if (score > MATE_THRESHOLD) return score + ply;
        if (score < -MATE_THRESHOLD) return score - ply;
        return score;
    }
    static int fromTT(int score, int ply) {
        if (score > MATE_THRESHOLD) return score - ply;
        if (score < -MATE_THRESHOLD) return score + ply;
        return score;
    }

    std::vector<TTEntry> table;
    uint64_t mask = 0;  // table.size() - 1, so indexing is a mask not a modulo
};