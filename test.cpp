#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include "board.hpp"
#include "movegen.hpp"
#include "types.hpp"
#include "zobrist.hpp"

// --- Behaviour Tests ---

static int failures = 0, checked = 0;

uint64_t perft(Board& board, int depth) {
    if (depth == 0) return 1;

    uint64_t scratchHash = getZobrist(board);
    if (board.zobristHash != scratchHash) {
        std::cerr << "\nFATAL HASH MISMATCH!\n"
                  << "Incremental : " << board.zobristHash << '\n'
                  << "Scratch     : " << scratchHash << '\n'
                  << "FEN         : " << board.toFEN() << "\n\n";
        assert(false);
    }

    MoveList moves;
    generateLegal(board, moves);
    if (depth == 1) return moves.size();
    uint64_t nodes = 0;
    MoveUndo undo;
    for (Move m : moves) {
        board.makeMove(m, undo);
        nodes += perft(board, depth - 1);
        board.unmakeMove(m, undo);
    }
    return nodes;
}

struct PerftCase {
    const char* fen;
    uint64_t expected[6];   // depths 1..6, 0 means "don't test"
};

static const PerftCase PERFT_CASES[] = {
    // 1. starting position
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
     {20, 400, 8902, 197281, 4865609, 119060324}},

    // 2. Kiwipete -- castling, pins, promotions
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
     {48, 2039, 97862, 4085603, 193690690, 0}},

    // 3. en passant edge cases, discovered checks
    {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
     {14, 191, 2812, 43238, 674624, 11030083}},

    // 4. promotions under check
    {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
     {6, 264, 9467, 422333, 15833292, 0}},

    // 5. tricky promotion / castling interactions
    {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
     {44, 1486, 62379, 2103487, 89941194, 0}},

    // 6. dense middlegame
    {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
     {46, 2079, 89890, 3894594, 0, 0}},
};

void runPerftTests(int maxDepth = 4) { // verifies move gen via comparision in number of moves
    const auto start = std::chrono::steady_clock::now();
    uint64_t totalNodes{0};
    for (const PerftCase& tc : PERFT_CASES) {
        Board b;
        b.fromFEN(tc.fen);
        for (int d = 1; d <= maxDepth; ++d) {
            uint64_t want = tc.expected[d - 1];
            if (want == 0) continue;
            uint64_t got = perft(b, d);
            totalNodes += got;
            ++checked;
            if (got != want) {
                std::cout << "FAIL perft(" << d << ")  got " << got
                          << "  want " << want << "\n      " << tc.fen << '\n';
                ++failures;
            }
        }
    }
    const auto end = std::chrono::steady_clock::now();
    const double seconds = std::chrono::duration<double>(end - start).count();
    std::cout << std::format("{:6.2f} Mnps\n", totalNodes / seconds / 1e6);
}

int main() {
    runPerftTests(4);
    std::cout << checked << " checks, " << failures << " failed\n";

    return failures != 0;
}