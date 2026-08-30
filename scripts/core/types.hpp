#pragma once
#include <array>
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
};

// easy indexing of pieces
inline constexpr std::array<Piece, 12> PIECE_LOOKUP = {
    W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING
};

constexpr Piece makePiece(PieceColour pc, PieceType pt) { return Piece((pc << 3) | pt); }
constexpr PieceType typeOf(Piece p) { return PieceType(p & 7); }
constexpr PieceColour colourOf(Piece p) { return PieceColour(p >> 3); }
constexpr PieceColour other(PieceColour pc) { return PieceColour(pc ^ 1);}

enum CastlingRight : uint8_t {
    NO_CASTLING = 0, WHITE_OO = 1, WHITE_OOO = 2, BLACK_OO = 4, BLACK_OOO = 8
};

constexpr uint8_t NO_SQUARE = 64;
constexpr int BOARD_SIZE = 64;
constexpr int FILE_COUNT = 8;
constexpr int RANK_COUNT = 8;

// --- FEN / Format ---

constexpr char FEN_CHARS[] = " PNBRQK  pnbrqk";

inline Piece pieceFromChar(char c) {
    const char* p = std::strchr(FEN_CHARS, c);
    return (p && c != ' ') ? Piece(p - FEN_CHARS) : NO_PIECE;
}

enum MoveFlag : uint8_t { // & CAPTURE (works for all), & 8 promotions only, & 3 gives promo piece
    QUIET = 0,
    DOUBLE_PUSH = 1,
    CASTLE_KING = 2,
    CASTLE_QUEEN = 3,
    CAPTURE = 4,
    EN_PASSANT = 5,

    PROM_KNIGHT     = 8,
    PROM_BISHOP     = 9,
    PROM_ROOK       = 10,
    PROM_QUEEN      = 11,
    PROM_KNIGHT_CAP = 12,
    PROM_BISHOP_CAP = 13,
    PROM_ROOK_CAP   = 14,
    PROM_QUEEN_CAP  = 15,
};

struct Move {
    uint16_t data = 0; // first 6 from square, second 6 to square, final is 4 bit flags

    constexpr int from() const { return data & 63; }
    constexpr int to() const { return  (data >> 6) & 63; }
    constexpr MoveFlag flag() const { return MoveFlag(data >> 12); }

    constexpr bool isCapture() const { return flag() & CAPTURE; }
    constexpr bool isPromotion() const { return flag() & 8; }
    constexpr PieceType promType() const { return PieceType(KNIGHT + (flag() & 3)); }

    constexpr bool operator==(const Move&) const = default;
};

struct MoveUndo {
    Piece captured;
    uint8_t castling;
    uint8_t epSquare;
    uint8_t halfMoveClock;
    uint64_t zobristHash;
};

constexpr Move encodeMove(int from, int to, MoveFlag f) {
    return Move{ uint16_t(from | (to << 6) | (f << 12)) };
}

constexpr Move NO_MOVE{0};

constexpr std::array<uint8_t, BOARD_SIZE> makeCastlingMasks() {
    std::array<uint8_t, BOARD_SIZE> m{};
    for (auto& x : m) x = WHITE_OO | WHITE_OOO | BLACK_OO | BLACK_OOO;
    m[0]  = uint8_t(m[0]  & ~WHITE_OOO);              // a1 rook
    m[7]  = uint8_t(m[7]  & ~WHITE_OO);               // h1 rook
    m[4]  = uint8_t(m[4]  & ~(WHITE_OO | WHITE_OOO)); // e1 king
    m[56] = uint8_t(m[56] & ~BLACK_OOO);              // a8 rook
    m[63] = uint8_t(m[63] & ~BLACK_OO);               // h8 rook
    m[60] = uint8_t(m[60] & ~(BLACK_OO | BLACK_OOO)); // e8 king
    return m;
}

inline constexpr auto CASTLING_MASKS = makeCastlingMasks();