#include "movegen.hpp"
#include "attacks.hpp"
#include "bitboard.hpp"
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

    explicit GenContext(const Board& b)
    : board(b),
        us(b.toMove),
        them(other(b.toMove)),
        ours(b.byColour[us]),
        theirs(b.byColour[them]),
        occupied(ours | theirs),
        notOurs(~ours) 
    {}
};

bool inCheck(const Board& board, PieceColour mover) {
    return isAttacked(board, lsb(board.pieces(mover, KING)), other(mover));
}

void addPawnMove(MoveList& moves, int from, int to, bool capture, int promoRank) {
    if (rankOf(to) == promoRank) {
        moves.push_back(makeMove(from, to, capture ? PROM_KNIGHT_CAP : PROM_KNIGHT));
        moves.push_back(makeMove(from, to, capture ? PROM_BISHOP_CAP : PROM_BISHOP));
        moves.push_back(makeMove(from, to, capture ? PROM_ROOK_CAP   : PROM_ROOK));
        moves.push_back(makeMove(from, to, capture ? PROM_QUEEN_CAP  : PROM_QUEEN));
    } else {
        moves.push_back(makeMove(from, to, capture ? CAPTURE : QUIET));
    }
}

void generatePawnMoves(const GenContext& c, MoveList& moves) {
    const int forward   = (c.us == WHITE) ? 8 : -8;
    const int startRank = (c.us == WHITE) ? 1 : 6;
    const int promoRank = (c.us == WHITE) ? 7 : 0;

    uint64_t pawns = c.board.byType[PAWN] & c.ours;
    while (pawns) {
        int from = popLsb(pawns);

        int one = from + forward;
        if (!(c.occupied & squareBB(one))) {
            addPawnMove(moves, from, one, false, promoRank);

            if (rankOf(from) == startRank) {
                int two = one + forward;
                if (!(c.occupied & squareBB(two)))
                    moves.push_back(makeMove(from, two, DOUBLE_PUSH));
            }
        }

        uint64_t attacks = PAWN_ATTACKS[c.us][from];
        uint64_t caps = attacks & c.theirs;
        while (caps) {
            int to = popLsb(caps);
            addPawnMove(moves, from, to, true, promoRank);
        }
        if (c.board.epSquare != NO_SQUARE && (attacks & squareBB(c.board.epSquare)))
            moves.push_back(makeMove(from, c.board.epSquare, EN_PASSANT));
    }
}

void generateKnightMoves(const GenContext& c, MoveList& moves) {
    uint64_t knights = c.board.pieces(c.us, KNIGHT);
    while (knights) {
        int from = popLsb(knights);
        uint64_t targets = KNIGHT_ATTACKS[from] & c.notOurs;
        while (targets) {
            int to = popLsb(targets);
            moves.push_back(makeMove(from, to,
                (c.theirs & squareBB(to)) ? CAPTURE : QUIET));
        }
    }
}

void generateKingMoves(const GenContext& c, MoveList& moves) {
    int from = lsb(c.board.pieces(c.us, KING)); // only one king
    uint64_t targets = KING_ATTACKS[from] & c.notOurs;
    while (targets) {
        int to = popLsb(targets);
        moves.push_back(makeMove(from, to,
            (c.theirs & squareBB(to)) ? CAPTURE : QUIET));
    }

    // --- castling ---
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
            moves.push_back(makeMove(from, g, CASTLE_KING));
    }

    if (c.board.castling & queenRight) {
        const int b = squareOf(1, rank), cc = squareOf(2, rank), d = squareOf(3, rank);
        const bool empty    = !(c.occupied & (squareBB(b) | squareBB(cc) | squareBB(d)));
        const bool pathSafe = !isAttacked(c.board, d, c.them)
                           && !isAttacked(c.board, cc, c.them);
        if (empty && pathSafe)
            moves.push_back(makeMove(from, cc, CASTLE_QUEEN));
    }
    
}

void generateSliderMoves(const GenContext& c, MoveList& moves) {
    uint64_t diagonal = (c.board.byType[BISHOP] | c.board.byType[QUEEN]) & c.ours;
    while (diagonal) {
        int from = popLsb(diagonal);
        uint64_t targets = bishopAttacks(from, c.occupied) & c.notOurs;
        while (targets) {
            int to = popLsb(targets);
            moves.push_back(makeMove(from, to,
                (c.theirs & squareBB(to)) ? CAPTURE : QUIET));
        }
    }

    uint64_t straight = (c.board.byType[ROOK] | c.board.byType[QUEEN]) & c.ours;
    while (straight) {
        int from = popLsb(straight);
        uint64_t targets = rookAttacks(from, c.occupied) & c.notOurs;
        while (targets) {
            int to = popLsb(targets);
            moves.push_back(makeMove(from, to,
                (c.theirs & squareBB(to)) ? CAPTURE : QUIET));
        }
    }
}

}  // namespace

// all legal moves, ignores if king in check
void generatePseudoLegal(const Board& board, MoveList& moves) {
    GenContext c(board);
    generatePawnMoves(c, moves);
    generateKingMoves(c, moves);
    generateSliderMoves(c, moves);
    generateKnightMoves(c, moves);
}

void generateLegal(const Board& board, MoveList& moves) {
    MoveList pseudo;
    generatePseudoLegal(board, pseudo);
    for (Move m : pseudo) {
        Board next = board.makeMove(m);
        if (!inCheck(next, board.toMove))
            moves.push_back(m);
    }
}