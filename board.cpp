#include "board.hpp"
#include "bitboard.hpp"
#include "types.hpp"
#include <cassert>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>

void Board::setPiece(int square, Piece p) {
    assert(p != NO_PIECE);

    mailbox[square] = p;
    // update bit boards
    byType[typeOf(p)] |= squareBB(square);
    byColour[colourOf(p)] |= squareBB(square);
}

void Board::removePiece(int square) {
    Piece p = at(square);
    if (p == NO_PIECE)
        return;

    mailbox[square] = NO_PIECE;
    // update bit boards
    byType[typeOf(p)] &= ~squareBB(square);
    byColour[colourOf(p)] &= ~squareBB(square);
}

// --- Output / Notation ---

void Board::fromFEN(const std::string &fen) {
    *this = Board{};      // zeros all elements
    epSquare = NO_SQUARE; // 0-63 is board

    std::istringstream ss(fen);
    std::string placement, side, castle, ep;
    int half = 0, full = 1;
    ss >> placement >> side >> castle >> ep >> half >> full; // splits up string fen

    int rank = RANK_COUNT - 1, file = 0;
    for (char c : placement) {
        if (c == '/') {
            rank--; // go down a row towards white
            file = 0;
        } else if (std::isdigit(static_cast<unsigned char>(c)))
            file += c - '0'; // converts to int
        else
            setPiece(squareOf(file++, rank), pieceFromChar(c));
    }

    toMove = (side == "w") ? WHITE : BLACK;

    for (char c : castle) {
        switch (c) {
        case 'K':
            castling |= WHITE_OO;
            break;
        case 'Q':
            castling |= WHITE_OOO;
            break;
        case 'k':
            castling |= BLACK_OO;
            break;
        case 'q':
            castling |= BLACK_OOO;
            break;
        }
    }

    if (ep != "-")
        epSquare = uint8_t(squareOf(ep[0] - 'a', ep[1] - '1'));

    halfMoveClock = uint8_t(half);
    fullMoveNumber = uint16_t(full);
}

std::string Board::toFEN() const {
    std::string s;

    for (int rank = RANK_COUNT - 1; rank >= 0; --rank) {
        int empty = 0;
        for (int file = 0; file < FILE_COUNT; ++file) {
            Piece p = at(squareOf(file, rank));
            if (p == NO_PIECE) {
                empty++;
                continue;
            }
            if (empty) {
                s += char('0' + empty);
                empty = 0;
            }
            s += FEN_CHARS[p];
        }
        if (empty)
            s += char('0' + empty);
        if (rank)
            s += '/';
    }

    s += ' ';
    s += (toMove == WHITE ? 'w' : 'b');
    s += ' ';

    if (!castling)
        s += '-';
    else {
        if (castling & WHITE_OO)
            s += 'K';
        if (castling & WHITE_OOO)
            s += 'Q';
        if (castling & BLACK_OO)
            s += 'k';
        if (castling & BLACK_OOO)
            s += 'q';
    }

    s += ' ';
    if (epSquare == NO_SQUARE)
        s += '-';
    else {
        s += char('a' + fileOf(epSquare));
        s += char('1' + rankOf(epSquare));
    }

    s += ' ' + std::to_string(halfMoveClock);
    s += ' ' + std::to_string(fullMoveNumber);
    return s;
}

void Board::printBoard() const {
    for (int rank = RANK_COUNT - 1; rank >= 0; --rank) {
        std::cerr << (rank + 1) << "  ";
        for (int file = 0; file < FILE_COUNT; ++file) {
            Piece p = at(squareOf(file, rank));
            std::cerr << (p == NO_PIECE ? '.' : FEN_CHARS[p]) << ' ';
        }
        std::cerr << '\n';
    }
    std::cerr << "\n   a b c d e f g h\n\nFEN: " << toFEN() << "\n\n";
}

Board Board::makeMove(Move m) const {
    Board next = *this; // copy
    Piece p = at(m.from());

    next.removePiece(m.from());

    if (m.isCapture()) {
        next.removePiece(m.flag() == EN_PASSANT ? squareOf(fileOf(m.to()), rankOf(m.from()))
                                                : m.to());
    }

    next.setPiece(m.to(), m.isPromotion() ? makePiece(next.toMove, m.promType()) : p);
    next.castling &= CASTLING_MASKS[m.from()] & CASTLING_MASKS[m.to()];

    if (m.flag() == CASTLE_KING) {
        int rank = rankOf(m.from());
        next.removePiece(squareOf(7, rank));
        next.setPiece(squareOf(5, rank), makePiece(colourOf(p), ROOK));
    } else if (m.flag() == CASTLE_QUEEN) {
        int rank = rankOf(m.from());
        next.removePiece(squareOf(0, rank));
        next.setPiece(squareOf(3, rank), makePiece(colourOf(p), ROOK));
    }

    next.epSquare = (m.flag() == DOUBLE_PUSH) ? uint8_t((m.from() + m.to()) >> 1) : NO_SQUARE;

    next.toMove = other(toMove);
    if (toMove == BLACK)
        next.fullMoveNumber++;
    next.halfMoveClock = (m.isCapture() || typeOf(p) == PAWN) ? 0 : uint8_t(next.halfMoveClock + 1);

    return next;
}
