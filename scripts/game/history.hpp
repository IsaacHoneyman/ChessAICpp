#pragma once
#include "types.hpp"
#include <cassert>
#include <cstdint>

struct PositionHistory {
    static constexpr int MAX_HISTORY =
        100 + MAX_SEARCH_PLY; // we only track since last capture or pawn push

    uint64_t hashes[MAX_HISTORY];
    int count = 0;
    int root = 0;

    void push(uint64_t h) {
        assert(count < MAX_HISTORY);
        hashes[count++] = h;
    }
    void pop() {
        assert(count > 0);
        --count;
    }
    void clear() {
        count = 0;
        root = 0;
    }

    void markRoot() { root = count; } // mark where search begins

    bool isRepetition(uint64_t hash, int halfMoveClock) const {
        const int oldest = (halfMoveClock < count) ? count - halfMoveClock : 0;
        for (int i = count - 2; i >= oldest; i-= 2) {
            if (hashes[i] == hash)
                return true;
        }

        return false;
    }

    bool isDoubleRepetition(uint64_t hash, int halfMoveClock) const {
        const int oldest = (halfMoveClock < count) ? count - halfMoveClock : 0;
        int seen = 0;
        for (int i = count - 2; i >= oldest; i -= 2) {
            if (hashes[i] == hash && ++seen == 2)
                return true;
        }
        return false;
    }
};