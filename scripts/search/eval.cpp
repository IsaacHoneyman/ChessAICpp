#include "eval.hpp"
#include "bitboard.hpp"
#include "board.hpp"
#include "types.hpp"

namespace {

inline constexpr Score BISHOP_PAIR = {28, 45}; // having both bishops is good
inline constexpr Score TEMPO = {18, 8}; // our turn to move is good

Score bishopPair(const Board& board) {
    Score s{};
    if (popCount(board.pieces(WHITE, BISHOP)) >= 2) s += BISHOP_PAIR;
    if (popCount(board.pieces(BLACK, BISHOP)) >= 2) s -= BISHOP_PAIR;
    return s;
}

Score tempo(const Board& board) {
    return board.toMove == WHITE ? TEMPO : -TEMPO;
}

int taper(Score s, int gamePhase) {
    const int phase = std::min(gamePhase, TOTAL_PHASE);
    return (s.mg * phase + s.eg * (TOTAL_PHASE - phase)) / TOTAL_PHASE;
}

// pawn structure

// Files either side
constexpr std::array<uint64_t, FILE_COUNT> makeAdjacentFiles() {
    std::array<uint64_t, FILE_COUNT> m{};
    for (int f = 0; f < FILE_COUNT; ++f) {
        if (f > 0) m[f] |= FILE_A << (f - 1);
        if (f < 7) m[f] |= FILE_A << (f + 1);
    }
    return m;
}

// Squares ahead of sq on its own file, from that colour's view.
constexpr std::array<std::array<uint64_t, BOARD_SIZE>, COLOUR_SIZE> makeForwardFile() {
    std::array<std::array<uint64_t, BOARD_SIZE>, COLOUR_SIZE> m{};
    for (int sq = 0; sq < BOARD_SIZE; ++sq) {
        const int f = fileOf(sq), r = rankOf(sq);
        for (int ahead = r + 1; ahead < RANK_COUNT; ++ahead)
            m[WHITE][sq] |= squareBB(squareOf(f, ahead));
        for (int ahead = r - 1; ahead >= 0; --ahead)
            m[BLACK][sq] |= squareBB(squareOf(f, ahead));
    }
    return m;
}

// Every square an enemy pawn could occupy to stop this one being passed:
constexpr std::array<std::array<uint64_t, BOARD_SIZE>, COLOUR_SIZE> makePassedMask() {
    std::array<std::array<uint64_t, BOARD_SIZE>, COLOUR_SIZE> m{};
    for (int sq = 0; sq < BOARD_SIZE; ++sq) {
        const int f = fileOf(sq), r = rankOf(sq);
        for (int file = std::max(0, f - 1); file <= std::min(7, f + 1); ++file) {
            for (int ahead = r + 1; ahead < RANK_COUNT; ++ahead)
                m[WHITE][sq] |= squareBB(squareOf(file, ahead));
            for (int ahead = r - 1; ahead >= 0; --ahead)
                m[BLACK][sq] |= squareBB(squareOf(file, ahead));
        }
    }
    return m;
}

constexpr std::array<uint64_t, FILE_COUNT> makeFileBB() {
    std::array<uint64_t, FILE_COUNT> m{};
    for (int f = 0; f < FILE_COUNT; ++f) m[f] = FILE_A << f;
    return m;
}

inline constexpr Score PASSED[RANK_COUNT] = { {0, 0}, {2, 5}, {4, 10}, {8, 18}, {15, 30}, {25, 55}, {40, 90}, {0, 0} };
inline constexpr Score DOUBLED  = {-10, -25}; // hurts more in an ending
inline constexpr Score ISOLATED = {-12, -18}; // no neighbour can ever defend it
inline constexpr Score ROOK_OPEN      = {20, 8}; // no pawns of either colour
inline constexpr Score ROOK_SEMI_OPEN = {15,  5}; // none of ours

inline constexpr auto FILE_BB        = makeFileBB();
inline constexpr auto ADJACENT_FILES = makeAdjacentFiles();
inline constexpr auto FORWARD_FILE   = makeForwardFile();
inline constexpr auto PASSED_MASK    = makePassedMask();

Score pawnStructure(const Board& board) {
    Score s{};
    for (const PieceColour c : {WHITE, BLACK}) {
        const uint64_t ours   = board.pieces(c, PAWN);
        const uint64_t theirs = board.pieces(other(c), PAWN);
 
        Score side{};
        uint64_t bb = ours;
        while (bb) {
            const int sq = popLsb(bb);
            const int rel = (c == WHITE) ? rankOf(sq) : 7 - rankOf(sq);
 
            if (!(PASSED_MASK[c][sq] & theirs))       side += PASSED[rel];
            if (!(ADJACENT_FILES[fileOf(sq)] & ours)) side += ISOLATED;
            if (FORWARD_FILE[c][sq] & ours)           side += DOUBLED;
        }
        s += (c == WHITE) ? side : -side;
    }
    return s;
}

Score rookFiles(const Board& board) {
    const uint64_t allPawns = board.byType[PAWN];
    Score s{};
    for (const PieceColour c : {WHITE, BLACK}) {
        const uint64_t ourPawns = board.pieces(c, PAWN);
 
        Score side{};
        uint64_t rooks = board.pieces(c, ROOK);
        while (rooks) {
            const uint64_t file = FILE_BB[fileOf(popLsb(rooks))];
            if (!(file & allPawns))      side += ROOK_OPEN;
            else if (!(file & ourPawns)) side += ROOK_SEMI_OPEN;
        }
        s += (c == WHITE) ? side : -side;
    }
    return s;
}

Score dynamicTerms(const Board& board) {
    Score s{};
    s += bishopPair(board);
    s += tempo(board);
    s += pawnStructure(board);
    s += rookFiles(board);
    return s; 
}

} // namespace

int evaluate(const Board& board) {
    const Score s = board.pstScore + dynamicTerms(board);
    const int score = taper(s, board.gamePhase);
    return board.toMove == WHITE ? score : -score;
}