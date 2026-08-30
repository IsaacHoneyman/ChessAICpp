#include "search.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "types.hpp"
#include <utility>

namespace {

constexpr int MAX_QUIESCE_PLY = 16; // termination for infinite checks

// most valuable victim with least valuable attacker
int captureScore(const Board &b, Move m) {
    const PieceType victim = (m.flag() == EN_PASSANT) ? PAWN : typeOf(b.at(m.to()));
    const PieceType attacker = typeOf(b.at(m.from()));
    int s = detail::VALUE[victim] * 16 - detail::VALUE[attacker]; // victim more important
    return s;
}

int moveScore(const Board& b, Move m) {
    if (m.isPromotion() && m.promType() == QUEEN) return 300'000 + (m.isCapture() ? captureScore(b, m) : 0);
    if (m.isCapture())   return 200'000 + captureScore(b, m);
    if (m.isPromotion()) return 100'000;   
    return 0; // killers, history go here
}

// select a move at a time, sorting tail is wasted work
void pickNext(MoveList& moves, int scores[], int start) {
    int best = start;
    for (int i = start + 1; i < moves.size(); ++i) {
        if (scores[i] > scores[best]) best = i;

    }
    if (best != start) {
        std::swap(moves.moves[start], moves.moves[best]);
        std::swap(scores[start], scores[best]);
    }
}

bool isDraw(const Game& g, int ply) {
    return ply > 0 && (g.board.halfMoveClock >= 100 || g.history.isRepetition(g.board.zobristHash, g.board.halfMoveClock));
}

} // namespace

int Searcher::quiesce(Game& g, int alpha, int beta, int ply, int qply) {
    ++nodes;

    const bool ic = inCheck(g.board, g.board.toMove);
    if (!ic) { // assume capture can be declined 
        const int stand = evaluate(g.board);
        if (stand >= beta) return stand; // opponent wont let us get here
        if (stand > alpha) alpha = stand; // new best move found
    }

    if (qply >= MAX_QUIESCE_PLY || ply >= MAX_SEARCH_PLY) return evaluate(g.board);

    MoveList moves;
    generateLegal(g.board, moves);

    if (moves.size() == 0) return ic ? -MATE + ply : 0; // mate or stalemate

    int scores[256];
    int considered = 0;
    for (int i = 0; i < moves.size(); ++i) {
        if (ic || moves.moves[i].isCapture() || moves.moves[i].isPromotion()) {
            moves.moves[considered] = moves.moves[i];
            scores[considered] = moveScore(g.board, moves.moves[i]);
            ++considered;
        }
    }
    moves.count = considered;

    MoveUndo undo;
    for (int i = 0; i < moves.size(); ++i) {
        pickNext(moves, scores, i);
        const Move m = moves.moves[i];

        g.makeMove(m, undo);
        const int score = -quiesce(g, -beta, -alpha, ply + 1, qply + 1);
        g.undoMove(m, undo);

        if (score >= beta) return score;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

int Searcher::negamax(Game& g, int depth, int ply, int alpha, int beta) {
    ++nodes;

    if (isDraw(g, ply)) return 0;
    if (depth <= 0) return quiesce(g, alpha, beta, ply, 1);

    MoveList moves;
    generateLegal(g.board, moves);

    if (moves.size() == 0) return inCheck(g.board, g.board.toMove) ? -MATE + ply : 0;

    int scores[256];
    for (int i = 0; i < moves.size(); ++i) scores[i] = moveScore(g.board, moves.moves[i]);

    MoveUndo undo;
    int best = -INF;
    for (int i = 0; i < moves.size(); ++i) {
        pickNext(moves, scores, i);
        const Move m = moves.moves[i];

        g.makeMove(m, undo);
        const int score = -negamax(g, depth - 1, ply + 1, -beta, -alpha);
        g.undoMove(m, undo);

        if (score > best) best = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) break; // opponent had better option
    }
    return best;
    
}

SearchResult Searcher::search(Game& g, const MoveList& root, int depth) {
    nodes = 0;
    if (root.size() == 0) return {};

    MoveList moves = root; // seperate as we need to remember move that produced score
    int scores[256];
    for (int i = 0; i < moves.size(); ++i) scores[i] = moveScore(g.board, moves.moves[i]);

    SearchResult r;
    r.move = moves.moves[0];
    r.depth = depth;

    int alpha = -INF;
    MoveUndo undo;

    for (int i = 0; i < moves.size(); ++i) {
        pickNext(moves, scores, i);
        const Move m = moves.moves[i];

        g.makeMove(m, undo);
        const int score = -negamax(g, depth - 1, 1, -INF, -alpha);
        g.undoMove(m, undo);

        if (i == 0 || score > alpha) {
            alpha = score;
            r.move = m;
            r.score = score;
        }
    }

    r.nodes = nodes;
    return r;
}

void Searcher::clear() {
    // nothing to be done yet
}





