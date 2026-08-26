#pragma once
#include <cstdint>
#include <cstring>

enum PieceColour : uint8_t { 
    WHITE = 0, 
    BLACK = 1, 
    COLOUR_SIZE = 2,
};

enum PieceType : uint8_t {
    NO_TYPE = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3,
    ROOK = 4, QUEEN = 5, KING = 6, TYPE_SIZE = 7
};

enum Piece : uint8_t { // represented eeee cttt
    NO_PIECE = 0,
    W_PAWN = 1, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = 9, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_SIZE = 16
};

constexpr Piece makePiece(PieceColour pc, PieceType pt) { return Piece((pc << 3) | pt); }
constexpr PieceType typeOf(Piece p) { return PieceType(p & 7); }
constexpr PieceColour colourOf(Piece p) { return PieceColour(p >> 3); }

enum CastlingRight : uint8_t {
    NO_CASTLING = 0, WHITE_OO = 1, WHITE_OOO = 2, BLACK_OO = 4, BLACK_OOO = 8
};

constexpr uint8_t NO_SQUARE = 64;

// --- FEN / Format ---

constexpr char FEN_CHARS[] = " PNBRQK  pnbrqk";

inline Piece pieceFromChar(char c) {
    const char* p = std::strchr(FEN_CHARS, c);
    return (p && c != ' ') ? Piece(p - FEN_CHARS) : NO_PIECE;
}