#pragma once
#include <cstdint>
#include <string>
#include "eval.hpp"
#include "types.hpp"

struct Board {
    uint64_t byType[TYPE_SIZE]; // type bitboards (7, including none)
    uint64_t byColour[COLOUR_SIZE]; // colour bitboards (2)
    uint8_t  mailbox[64];   

    uint64_t zobristHash;
    uint8_t castling;
    uint8_t epSquare; // NO_SQUARE
    uint8_t halfMoveClock; // 50 move rule
    uint16_t fullMoveNumber; 
    PieceColour toMove;

    Score pstScore;
    int32_t gamePhase;

    uint64_t occupied() const { return byColour[WHITE] | byColour[BLACK]; }
    uint64_t pieces(PieceColour c, PieceType pt) const { return byColour[c] & byType[pt]; }
    Piece at(int square) const { return Piece(mailbox[square]); }

    void setPiece(int square, Piece p);
    void removePiece(int square);

    void makeMove(Move m, MoveUndo& undo);
    void unmakeMove(Move m, const MoveUndo& undo);

    // --- Output / Notation ---

    void fromFEN(const std::string& fen);
    std::string toFEN() const;
    void printBoard() const;
};