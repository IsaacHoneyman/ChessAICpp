#include "notation.hpp"
#include "bitboard.hpp"
#include "types.hpp"

namespace {

constexpr bool validFile(char c) { return c >= 'a' && c <= 'h'; }
constexpr bool validRank(char c) { return c >= '1' && c <= '8'; }

constexpr PieceType promoFromChar(char c) {
    switch (c) {
    case 'n': return KNIGHT;
    case 'b': return BISHOP;
    case 'r': return ROOK;
    case 'q': return QUEEN;
    default:  return NO_TYPE;
    }
}

} // namespace

std::string toUCI(Move m) {
    if (m == NO_MOVE) return "0000";
 
    std::string s;
    s += char('a' + fileOf(m.from()));
    s += char('1' + rankOf(m.from()));
    s += char('a' + fileOf(m.to()));
    s += char('1' + rankOf(m.to()));
    // black's half of FEN_CHARS is already the lowercase promotion letters
    if (m.isPromotion()) s += FEN_CHARS[makePiece(BLACK, m.promType())];
    return s;
}

Move fromUCI(const MoveList& legal, std::string_view s) {
    if (s.size() < 4 || s.size() > 5) return NO_MOVE;
    if (!validFile(s[0]) || !validRank(s[1])) return NO_MOVE;
    if (!validFile(s[2]) || !validRank(s[3])) return NO_MOVE;
 
    const int from = squareOf(s[0] - 'a', s[1] - '1');
    const int to   = squareOf(s[2] - 'a', s[3] - '1');
 
    const PieceType promo = (s.size() == 5) ? promoFromChar(s[4]) : NO_TYPE;
    if (s.size() == 5 && promo == NO_TYPE) return NO_MOVE;
 
    for (const Move m : legal) {
        if (m.from() != from || m.to() != to) continue;
        if (m.isPromotion() != (promo != NO_TYPE)) continue;
        if (promo != NO_TYPE && m.promType() != promo) continue;
        return m;
    }
    return NO_MOVE;
}