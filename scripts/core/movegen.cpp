#include "movegen.hpp"
#include "attacks.hpp"
#include "bitboard.hpp"
#include "history.hpp"
#include "types.hpp"
#include <cstdint>

namespace {

struct GenContext {
    const Board& board;
    PieceColour us;
    PieceColour them;
    uint64_t ours;
    uint64_t theirs;
    uint64_t occupied;
    uint64_t notOurs;
    uint64_t targets;   // squares a piece may land on
    bool capturesOnly;

    GenContext(const Board& b, GenType type)
    : board(b),
        us(b.toMove),
        them(other(b.toMove)),
        ours(b.byColour[us]),
        theirs(b.byColour[them]),
        occupied(ours | theirs),
        notOurs(~ours),
        targets(type == GenType::CAPTURES ? theirs : notOurs),
        capturesOnly(type == GenType::CAPTURES)
    {}
};

// Enemy pieces currently giving check.
uint64_t computeCheckers(const Board& b, PieceColour us, int kingSq) {
    const uint64_t occ = b.occupied();
    return b.byColour[other(us)] & (
        (b.byType[PAWN]   & PAWN_ATTACKS[us][kingSq]) |
        (b.byType[KNIGHT] & KNIGHT_ATTACKS[kingSq])   |
        ((b.byType[BISHOP] | b.byType[QUEEN]) & bishopAttacks(kingSq, occ)) |
        ((b.byType[ROOK]   | b.byType[QUEEN]) & rookAttacks(kingSq, occ)));
}

// Our pieces sitting alone between our king and an enemy slider.
uint64_t computePinned(const Board& b, PieceColour us, int kingSq) {
    const uint64_t occ  = b.occupied();
    const uint64_t them = b.byColour[other(us)];

    uint64_t snipers = them & (
        ((b.byType[ROOK]   | b.byType[QUEEN]) & rookAttacks(kingSq, 0)) |
        ((b.byType[BISHOP] | b.byType[QUEEN]) & bishopAttacks(kingSq, 0)));

    uint64_t pinned = 0;
    while (snipers) {
        const int sq = popLsb(snipers);
        const uint64_t blockers = BETWEEN[kingSq][sq] & occ;
        if (popCount(blockers) == 1)
            pinned |= blockers & b.byColour[us]; 
    }
    return pinned;
}

// Set-wise pawn generation: shift the whole pawn set at once rather than looping
void generatePawnMoves(const GenContext& c, MoveList& moves) {
    const bool white = (c.us == WHITE);
    const uint64_t pawns = c.board.byType[PAWN] & c.ours;
    const uint64_t empty = ~c.occupied;

    const uint64_t promoRank  = white ? RANK_8 : RANK_1;
    const uint64_t doubleRank = white ? RANK_3 : RANK_6;

    const int up   = white ?  8 : -8;   // single push
    const int upE  = white ?  9 : -7;   // capture toward the h-file
    const int upW  = white ?  7 : -9;   // capture toward the a-file

    auto shiftUp = [&](uint64_t bb) { return white ? bb << 8 : bb >> 8; };
    auto shiftE  = [&](uint64_t bb) { return white ? (bb & ~FILE_H) << 9 : (bb & ~FILE_H) >> 7; };
    auto shiftW  = [&](uint64_t bb) { return white ? (bb & ~FILE_A) << 7 : (bb & ~FILE_A) >> 9; };

    // quite pushes
    if (!c.capturesOnly) {
        uint64_t single = shiftUp(pawns) & empty & ~promoRank;
        uint64_t dbl    = shiftUp(shiftUp(pawns) & empty & doubleRank) & empty;

        while (single) {
            const int to = popLsb(single);
            moves.push_back(encodeMove(to - up, to, QUIET));
        }
        while (dbl) {
            const int to = popLsb(dbl);
            moves.push_back(encodeMove(to - 2 * up, to, DOUBLE_PUSH));
        }
    }

    // captures
    uint64_t capE = shiftE(pawns) & c.theirs & ~promoRank;
    while (capE) {
        const int to = popLsb(capE);
        moves.push_back(encodeMove(to - upE, to, CAPTURE));
    }

    uint64_t capW = shiftW(pawns) & c.theirs & ~promoRank;
    while (capW) {
        const int to = popLsb(capW);
        moves.push_back(encodeMove(to - upW, to, CAPTURE));
    }

    // promos
    const uint64_t promoters = pawns & (white ? RANK_7 : RANK_2);
    if (promoters) {
        uint64_t push = shiftUp(promoters) & empty;
        while (push) {
            const int to = popLsb(push), from = to - up;
            moves.push_back(encodeMove(from, to, PROM_KNIGHT));
            moves.push_back(encodeMove(from, to, PROM_BISHOP));
            moves.push_back(encodeMove(from, to, PROM_ROOK));
            moves.push_back(encodeMove(from, to, PROM_QUEEN));
        }

        uint64_t pcE = shiftE(promoters) & c.theirs;
        while (pcE) {
            const int to = popLsb(pcE), from = to - upE;
            moves.push_back(encodeMove(from, to, PROM_KNIGHT_CAP));
            moves.push_back(encodeMove(from, to, PROM_BISHOP_CAP));
            moves.push_back(encodeMove(from, to, PROM_ROOK_CAP));
            moves.push_back(encodeMove(from, to, PROM_QUEEN_CAP));
        }

        uint64_t pcW = shiftW(promoters) & c.theirs;
        while (pcW) {
            const int to = popLsb(pcW), from = to - upW;
            moves.push_back(encodeMove(from, to, PROM_KNIGHT_CAP));
            moves.push_back(encodeMove(from, to, PROM_BISHOP_CAP));
            moves.push_back(encodeMove(from, to, PROM_ROOK_CAP));
            moves.push_back(encodeMove(from, to, PROM_QUEEN_CAP));
        }
    }

    // --- en passant ---
    if (c.board.epSquare != NO_SQUARE) {
        uint64_t eps = PAWN_ATTACKS[c.them][c.board.epSquare] & pawns;
        while (eps)
            moves.push_back(encodeMove(popLsb(eps), c.board.epSquare, EN_PASSANT));
    }
}

void generateKnightMoves(const GenContext& c, MoveList& moves) {
    uint64_t knights = c.board.pieces(c.us, KNIGHT);
    while (knights) {
        int from = popLsb(knights);
        uint64_t targets = KNIGHT_ATTACKS[from] & c.targets;
        while (targets) {
            int to = popLsb(targets);
            moves.push_back(encodeMove(from, to,
                (c.theirs & squareBB(to)) ? CAPTURE : QUIET));
        }
    }
}

void generateKingMoves(const GenContext& c, MoveList& moves) {
    int from = lsb(c.board.pieces(c.us, KING)); // only one king
    uint64_t targets = KING_ATTACKS[from] & c.targets;
    while (targets) {
        int to = popLsb(targets);
        moves.push_back(encodeMove(from, to,
            (c.theirs & squareBB(to)) ? CAPTURE : QUIET));
    }

    // --- castling ---
    // never capture so can skip.
    if (c.capturesOnly) return;

    const int rank           = (c.us == WHITE) ? 0 : 7;
    const uint8_t kingRight  = (c.us == WHITE) ? WHITE_OO  : BLACK_OO;
    const uint8_t queenRight = (c.us == WHITE) ? WHITE_OOO : BLACK_OOO;

    if (!(c.board.castling & (kingRight | queenRight))) return;
    if (isAttacked(c.board, from, c.them)) return;      // can't castle out of check

    if (c.board.castling & kingRight) {
        const int f = squareOf(5, rank), g = squareOf(6, rank);
        const bool empty    = !(c.occupied & (squareBB(f) | squareBB(g)));
        const bool pathSafe = !isAttacked(c.board, f, c.them)
                           && !isAttacked(c.board, g, c.them);
        if (empty && pathSafe)
            moves.push_back(encodeMove(from, g, CASTLE_KING));
    }

    if (c.board.castling & queenRight) {
        const int b = squareOf(1, rank), cc = squareOf(2, rank), d = squareOf(3, rank);
        const bool empty    = !(c.occupied & (squareBB(b) | squareBB(cc) | squareBB(d)));
        const bool pathSafe = !isAttacked(c.board, d, c.them)
                           && !isAttacked(c.board, cc, c.them);
        if (empty && pathSafe)
            moves.push_back(encodeMove(from, cc, CASTLE_QUEEN));
    }
    
}

void generateSliderMoves(const GenContext& c, MoveList& moves) {
    uint64_t diagonal = (c.board.byType[BISHOP] | c.board.byType[QUEEN]) & c.ours;
    while (diagonal) {
        int from = popLsb(diagonal);
        uint64_t targets = bishopAttacks(from, c.occupied) & c.targets;
        while (targets) {
            int to = popLsb(targets);
            moves.push_back(encodeMove(from, to,
                (c.theirs & squareBB(to)) ? CAPTURE : QUIET));
        }
    }

    uint64_t straight = (c.board.byType[ROOK] | c.board.byType[QUEEN]) & c.ours;
    while (straight) {
        int from = popLsb(straight);
        uint64_t targets = rookAttacks(from, c.occupied) & c.targets;
        while (targets) {
            int to = popLsb(targets);
            moves.push_back(encodeMove(from, to,
                (c.theirs & squareBB(to)) ? CAPTURE : QUIET));
        }
    }
}

bool insufficientMaterial(const Board& board) {
    const int pieces = popCount(board.occupied());
    if (pieces > 4) return false;

    if (board.byType[PAWN] | board.byType[ROOK] | board.byType[QUEEN]) return false;

    if (pieces <= 3) return true;  // KK, or KK plus one minor

    // Four pieces: two kings and two others. Insufficient only if both are
    // bishops on the same colour square.
    const uint64_t bishops = board.byType[BISHOP];
    if (popCount(bishops) != 2) return false;

    return (bishops & LIGHT_SQUARES) == bishops    // both light
        || (bishops & LIGHT_SQUARES) == 0;         // both dark
}

}  // namespace

bool inCheck(const Board& board, PieceColour mover) {
    return isAttacked(board, lsb(board.pieces(mover, KING)), other(mover));
}

GameState getState(const Board& board, const MoveList& moves, const PositionHistory& history) {
    if (moves.size() == 0) {
        return inCheck(board, board.toMove) ? GameState::CHECKMATE : GameState::STALEMATE;
    }

    if (insufficientMaterial(board)) return GameState::INSUFFICIENT;
    if (history.isDoubleRepetition(board.zobristHash, board.halfMoveClock)) return GameState::REPETITION;

    return board.halfMoveClock >= 100 ? GameState::FIFTYMOVE : GameState::ONGOING;
}

// all legal moves, ignores if king in check
void generatePseudoLegal(const Board& board, MoveList& moves, GenType type) {
    GenContext c(board, type);
    generatePawnMoves(c, moves);
    generateKingMoves(c, moves);
    generateSliderMoves(c, moves);
    generateKnightMoves(c, moves);
}

void generateLegal(Board& board, MoveList& moves, GenType type) {
    moves.count = 0;

    const PieceColour us = board.toMove;
    const int kingSq = lsb(board.pieces(us, KING));
    const uint64_t checkers = computeCheckers(board, us, kingSq);

    // Evasions include quiet blocks and king moves to empty squares, so a
    // captures-only list would be incomplete and could look like mate.
    if (checkers) type = GenType::ALL;

    MoveList pseudo;
    generatePseudoLegal(board, pseudo, type);

    const uint64_t pinned = computePinned(board, us, kingSq);

    MoveUndo undo;
    for (Move m : pseudo) {
        const bool mustVerify = 
        checkers || m.from() == kingSq || m.flag() == EN_PASSANT || (pinned & squareBB(m.from()));

        if (!mustVerify) {
            moves.push_back(m);
            continue;
        }

        board.makeMove(m, undo);
        if (!inCheck(board, us))
            moves.push_back(m);
        board.unmakeMove(m, undo);
    }
}