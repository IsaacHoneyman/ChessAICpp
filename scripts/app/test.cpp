#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include "bitboard.hpp"
#include "board.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "types.hpp"
#include "zobrist.hpp"

// --- Behaviour Tests ---

namespace {

static int failures = 0, checked = 0;

uint64_t perft(Board& board, int depth) {
    if (depth == 0) return 1;
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
    const std::string fen;
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

void playMoves(Board& b, std::initializer_list<Move> moves) {
    MoveUndo undo;
    for (Move m : moves) b.makeMove(m, undo);
}

void expectHash(uint64_t got, uint64_t want, const std::string& what) {
    ++checked;
    if (got != want) {
        std::cout << "FAIL " << what << "\n  got  " << got << "\n  want " << want << '\n';
        ++failures;
    }
}

void runZobristTest() {
    const std::string START = PERFT_CASES[0].fen;

    constexpr int B1 = squareOf(1, 0), C3 = squareOf(2, 2), G1 = squareOf(6, 0), F3 = squareOf(5, 2);
    constexpr int B8 = squareOf(1, 7), C6 = squareOf(2, 5), G8 = squareOf(6, 7), F6 = squareOf(5, 5);
    constexpr int E2 = squareOf(4, 1), E4 = squareOf(4, 3);

    // --- path independence ---
    // Two different knight shuffles, both returning to the exact start position.
    Board viaKing; viaKing.fromFEN(START);
    playMoves(viaKing, {encodeMove(G1, F3, QUIET), encodeMove(G8, F6, QUIET),
                        encodeMove(F3, G1, QUIET), encodeMove(F6, G8, QUIET)});

    Board viaQueen; viaQueen.fromFEN(START);
    playMoves(viaQueen, {encodeMove(B1, C3, QUIET), encodeMove(B8, C6, QUIET),
                         encodeMove(C3, B1, QUIET), encodeMove(C6, B8, QUIET)});

    expectHash(viaKing.zobristHash, viaQueen.zobristHash, "path independence: two shuffles");

    // Both must also equal a fresh start position, or they're consistently wrong together.
    Board fresh; fresh.fromFEN(START);
    expectHash(viaKing.zobristHash, fresh.zobristHash, "shuffle returns to start hash");

    // --- ep canonicalisation ---
    // e2e4 from the start: no black pawn can capture on e3, so no ep square.
    Board pushed; pushed.fromFEN(START);
    playMoves(pushed, {encodeMove(E2, E4, DOUBLE_PUSH)});
    ++checked;
    if (pushed.epSquare != NO_SQUARE) {
        std::cout << "FAIL ep set with no capturer: " << pushed.toFEN() << '\n';
        ++failures;
    }

    // Same push with a black pawn on d4, which can capture on e3 — ep must be set.
    Board capturable;
    capturable.fromFEN("rnbqkbnr/pppp1ppp/8/8/3p4/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    playMoves(capturable, {encodeMove(E2, E4, DOUBLE_PUSH)});
    ++checked;
    if (capturable.epSquare == NO_SQUARE) {
        std::cout << "FAIL ep cleared with capturer present: " << capturable.toFEN() << '\n';
        ++failures;
    }
    expectHash(capturable.zobristHash, getZobrist(capturable), "ep: incremental vs scratch");

    // --- FEN round trip ---
    Board reloaded; reloaded.fromFEN(capturable.toFEN());
    expectHash(reloaded.zobristHash, capturable.zobristHash, "FEN round trip with ep set");

    // --- make/unmake round trip ---
    Board b; b.fromFEN(START);
    const uint64_t before = b.zobristHash;
    MoveList moves;
    generateLegal(b, moves);
    MoveUndo undo;
    bool ok = true;
    for (Move m : moves) {
        b.makeMove(m, undo);
        b.unmakeMove(m, undo);
        if (b.zobristHash != before) ok = false;
    }
    ++checked;
    if (!ok) {
        std::cout << "FAIL make/unmake round trip from startpos\n";
        ++failures;
    }
}

void expectInt(int got, int want, const std::string& what) {
    ++checked;
    if (got != want) {
        std::cout << "FAIL " << what << "\n  got  " << got << "\n  want " << want << '\n';
        ++failures;
    }
}
 
std::string mirrorFEN(const std::string& fen) {
    std::istringstream ss(fen);
    std::string placement, side, castle, ep;
    int half = 0, full = 1;
    ss >> placement >> side >> castle >> ep >> half >> full;
 
    std::vector<std::string> ranks;
    for (size_t start = 0, p; ; start = p + 1) {
        p = placement.find('/', start);
        ranks.push_back(placement.substr(start, p - start));
        if (p == std::string::npos) break;
    }
    std::string flipped;
    for (auto it = ranks.rbegin(); it != ranks.rend(); ++it) {
        if (!flipped.empty()) flipped += '/';
        for (char c : *it)
            flipped += std::isalpha(static_cast<unsigned char>(c))
                     ? char(std::islower(static_cast<unsigned char>(c)) ? std::toupper(c)
                                                                       : std::tolower(c))
                     : c;
    }
 
    std::string newCastle;
    for (char c : castle)
        newCastle += std::isalpha(static_cast<unsigned char>(c))
                   ? char(std::islower(static_cast<unsigned char>(c)) ? std::toupper(c)
                                                                     : std::tolower(c))
                   : c;
 
    std::string newEp = ep;
    if (ep != "-") newEp = std::string(1, ep[0]) + char('1' + (7 - (ep[1] - '1')));
 
    return flipped + ' ' + (side == "w" ? "b" : "w") + ' ' + newCastle + ' ' + newEp
         + ' ' + std::to_string(half) + ' ' + std::to_string(full);
}
 
// Walks the subtree checking that everything maintained incrementally still
// matches a from-scratch recomputation. Returns a description of the first
// position that disagrees, or an empty string if the whole subtree is clean.
std::string findDrift(Board& b, int depth) {
    if (b.zobristHash != getZobrist(b))
        return "zobrist drift: " + b.toFEN();
    if (b.pstScore != evaluateFromScratch(b))
        return "pstScore drift: " + b.toFEN()
             + " (incremental " + std::to_string(b.pstScore)
             + ", scratch " + std::to_string(evaluateFromScratch(b)) + ')';
    if (depth == 0) return {};

    MoveList moves;
    generateLegal(b, moves);
    MoveUndo undo;
    for (const Move m : moves) {
        b.makeMove(m, undo);
        const std::string drift = findDrift(b, depth - 1);
        b.unmakeMove(m, undo);
        if (!drift.empty()) return drift;
    }
    return {};
}

void expectConsistentSubtree(const std::string& fen, int depth) {
    ++checked;
    Board b; b.fromFEN(fen);
    const std::string drift = findDrift(b, depth);
    if (!drift.empty()) {
        std::cout << "FAIL incremental state under " << fen << "\n  " << drift << '\n';
        ++failures;
    }
}
 
void runPstTests() {
    expectInt(PST[W_PAWN][squareOf(4, 1)], 80,  "white pawn e2 (blocks bishop)");
    expectInt(PST[W_PAWN][squareOf(4, 3)], 120, "white pawn e4 (centre)");
    expectInt(PST[W_PAWN][squareOf(4, 6)], 150, "white pawn e7 (near promotion)");
    expectInt(PST[W_KNIGHT][squareOf(0, 0)], 270, "white knight a1 (corner)");
    expectInt(PST[W_KNIGHT][squareOf(3, 3)], 340, "white knight d4 (centre)");
    expectInt(PST[W_KING][squareOf(6, 0)], 30,  "white king g1 (castled)");
    expectInt(PST[W_KING][squareOf(4, 3)], -40, "white king e4 (exposed)");
    expectInt(PST[B_PAWN][squareOf(4, 4)], -120, "black pawn e5 (centre)");
    expectInt(PST[B_KING][squareOf(6, 7)], -30,  "black king g8 (castled)");
 
    int asym = 0;
    for (const PieceType pt : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING})
        for (int sq = 0; sq < BOARD_SIZE; ++sq)
            if (PST[makePiece(WHITE, pt)][sq] != -PST[makePiece(BLACK, pt)][sq ^ 56]) ++asym;
    expectInt(asym, 0, "PST colour symmetry");
 
    int nonZero = 0;
    for (int sq = 0; sq < BOARD_SIZE; ++sq)
        for (const int p : {0, 7, 8}) if (PST[p][sq] != 0) ++nonZero;
    expectInt(nonZero, 0, "unused Piece slots are zero");
 
    Board start; start.fromFEN(PERFT_CASES[0].fen);
    expectInt(start.pstScore, 0, "startpos scores 0");
 
    for (const PerftCase& tc : PERFT_CASES) {
        Board a; a.fromFEN(tc.fen);
        Board m; m.fromFEN(mirrorFEN(tc.fen));
        expectInt(m.pstScore, -a.pstScore, "mirror negates score: " + tc.fen);
        expectInt(evaluate(m), evaluate(a), "mirror preserves evaluate(): " + tc.fen);
    }
 
    for (const PerftCase& tc : PERFT_CASES) expectConsistentSubtree(tc.fen, 3);
}

} // namespace

int main() {
    runZobristTest();
    runPerftTests(4);
    runPstTests();
    std::cout << checked << " checks, " << failures << " failed\n";

    return failures != 0;
}