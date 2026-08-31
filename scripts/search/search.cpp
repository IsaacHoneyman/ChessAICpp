#include "search.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "types.hpp"
#include <chrono>
#include <climits>
#include <cmath>
#include <utility>

namespace {

constexpr int MAX_QUIESCE_PLY = 16; // termination for infinite checks

constexpr std::array<std::array<int, TYPE_SIZE>, TYPE_SIZE> makeMvvLvaTable() {
    constexpr int VAL[TYPE_SIZE] = {0, 100, 300, 300, 500, 900, 0};
    std::array<std::array<int, TYPE_SIZE>, TYPE_SIZE> table{};
    for (int victim = 1; victim < TYPE_SIZE; ++victim) {
        for (int attacker = 1; attacker < TYPE_SIZE; ++attacker) {
            table[victim][attacker] = VAL[victim] * 16 - VAL[attacker];
        }
    }
    return table;
}

constexpr auto MVV_LVA = makeMvvLvaTable();

// most valuable victim with least valuable attacker
int captureScore(const Board &b, Move m) {
    const PieceType victim = (m.flag() == EN_PASSANT) ? PAWN : typeOf(b.at(m.to()));
    const PieceType attacker = typeOf(b.at(m.from()));
    return MVV_LVA[victim][attacker];
}

constexpr int SCORE_TT = 600'000;
constexpr int SCORE_Q_PROMO = 500'000;
constexpr int SCORE_CAPTURE = 400'000;
constexpr int SCORE_KILLER_1 = 300'000;
constexpr int SCORE_KILLER_2 = 200'000;
constexpr int SCORE_UNDERPROMO = 100'000;
constexpr int HISTORY_MAX = 90'000; // stay under promo

// --- Late move reductions ---
constexpr int LMR_MIN_DEPTH = 3; // below this there is nothing to reduce
constexpr int LMR_MIN_MOVE = 3;  // first three moves always get full depth

std::array<std::array<int, 64>, 64> LMR_TABLE{};

bool initLmrTable() { // not constant expr as std::log can't be done at compile time, would work in c++ 26
    for (int d = 1; d < 64; ++d) {
        for (int m = 1; m < 64; ++m) {
            // Standard logarithmic scaling constants: base 0.75, divisor 2.25
            const double reduction = 0.75 + std::log(d) * std::log(m) / 2.25;
            LMR_TABLE[d][m] = static_cast<int>(reduction);
        }
    }
    return true;
}
const bool lmr_initialized = initLmrTable();

int reduction(int depth, int moveIndex) {
    return LMR_TABLE[std::min(depth, 63)][std::min(moveIndex, 63)];
}

int moveScore(const Board &b, Move m, Move ttMove, const Move *killers, const int (*history)[64]) {
    if (m == ttMove)
        return SCORE_TT; // best move from a previous search
    if (m.isPromotion() && m.promType() == QUEEN)
        return SCORE_Q_PROMO + (m.isCapture() ? captureScore(b, m) : 0);
    if (m.isCapture())
        return SCORE_CAPTURE + captureScore(b, m);
    if (killers) {
        if (m == killers[0])
            return SCORE_KILLER_1;
        if (m == killers[1])
            return SCORE_KILLER_2;
    }
    if (m.isPromotion())
        return SCORE_UNDERPROMO;
    return history ? history[b.at(m.from())][m.to()] : 0;
}

// select a move at a time, sorting tail is wasted work
void pickNext(MoveList &moves, int scores[], int start) {
    int best = start;
    for (int i = start + 1; i < moves.size(); ++i) {
        if (scores[i] > scores[best])
            best = i;
    }
    if (best != start) {
        std::swap(moves.moves[start], moves.moves[best]);
        std::swap(scores[start], scores[best]);
    }
}

bool hasNonPawnMaterial(const Board &b, PieceColour c) {
    return (b.byColour[c] & (b.byType[KNIGHT] | b.byType[BISHOP] |
                             b.byType[ROOK]   | b.byType[QUEEN])) != 0;
}

bool isDraw(const Game &g, int ply) {
    return ply > 0 && (g.board.halfMoveClock >= 100 ||
                       g.history.isRepetition(g.board.zobristHash, g.board.halfMoveClock));
}

} // namespace

int Searcher::quiesce(Game &g, int alpha, int beta, int ply, int qply) {
    ++nodes;
    ++qnodes;
    if (ply > seldepth)
        seldepth = ply;
    if (outOfTime())
        return 0; // discard iteration

    const bool ic = inCheck(g.board, g.board.toMove);
    if (!ic) { // assume capture can be declined
        const int stand = evaluate(g.board);
        if (stand >= beta)
            return stand; // opponent wont let us get here
        if (stand > alpha)
            alpha = stand; // new best move found
    }

    if (qply >= MAX_QUIESCE_PLY || ply >= MAX_SEARCH_PLY)
        return evaluate(g.board);

    MoveList moves;
    generateLegal(g.board, moves);

    if (moves.size() == 0)
        return ic ? -MATE + ply : 0; // mate or stalemate

    int scores[256];
    int considered = 0;
    for (int i = 0; i < moves.size(); ++i) {
        if (ic || moves.moves[i].isCapture() || moves.moves[i].isPromotion()) {
            moves.moves[considered] = moves.moves[i];
            scores[considered] = moveScore(g.board, moves.moves[i], NO_MOVE, nullptr, nullptr);
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

        if (score >= beta)
            return score;
        if (score > alpha)
            alpha = score;
    }
    return alpha;
}

int Searcher::negamax(Game &g, int depth, int ply, int alpha, int beta, bool allowNmp) {
    ++nodes;
    if (ply > seldepth)
        seldepth = ply;
    if (outOfTime())
        return 0; // discard iteration

    if (isDraw(g, ply))
        return 0;

    const int alphaOrig = alpha;
    Move ttMove = NO_MOVE;
    int ttScore = 0;
    if (ply > 0 && tt.probe(g.board.zobristHash, depth, ply, alpha, beta, ttScore, ttMove))
        return ttScore;

    if (depth <= 0)
        return quiesce(g, alpha, beta, ply, 1);

    const bool inCheckNow = inCheck(g.board, g.board.toMove);

    if (allowNmp && !inCheckNow && depth >= 3 && hasNonPawnMaterial(g.board, g.board.toMove)) {
        const int eval = evaluate(g.board);

        if (eval >= beta) {
            MoveUndo nullUndo;
            g.board.makeNullMove(nullUndo);
            int R = (3 + depth / 6) + (eval - beta > 200 ? 1 : 0);
            const int nullScore = -negamax(g, std::max(0, depth - R), ply + 1, -beta, -beta + 1, false);
            g.board.unmakeNullMove(nullUndo);

            if (stopped)
                return 0;

            if (nullScore >= beta) {
                return nullScore >= MATE_THRESHOLD ? beta : nullScore;
            }
        }
    }

    MoveList moves;
    generateLegal(g.board, moves);

    if (moves.size() == 0)
        return inCheckNow ? -MATE + ply : 0;

    int scores[256];
    for (int i = 0; i < moves.size(); ++i)
        scores[i] = moveScore(g.board, moves.moves[i], ttMove, killers[ply], history);

    MoveUndo undo;
    int best = -INF;
    Move bestMove = NO_MOVE;
    for (int i = 0; i < moves.size(); ++i) {
        pickNext(moves, scores, i);
        const Move m = moves.moves[i];
        const bool quiet = !m.isCapture() && !m.isPromotion();

        g.makeMove(m, undo);

        int score;
        if (quiet && !inCheckNow && depth >= LMR_MIN_DEPTH &&
            i >= LMR_MIN_MOVE) { // we try reduced depth
            const int r = reduction(depth, i);
            score = -negamax(g, depth - 1 - r, ply + 1, -alpha - 1, -alpha);

            if (score > alpha)
                score = -negamax(g, depth - 1, ply + 1, -beta, -alpha);
        } else {
            score = -negamax(g, depth - 1, ply + 1, -beta, -alpha);
        }

        g.undoMove(m, undo);

        if (score > best) {
            best = score;
            bestMove = m;
        }
        if (score > alpha)
            alpha = score;
        if (alpha >= beta) { // opponent had better option
            ++cutoffs;
            if (i == 0)
                ++firstCutoffs; // best move was tried first

            if (quiet) {
                const Piece p = g.board.at(m.from());
                history[p][m.to()] += depth * depth;
                if (history[p][m.to()] > HISTORY_MAX)
                    ageHistory();
                if (!(m == killers[ply][0])) {
                    killers[ply][1] = killers[ply][0];
                    killers[ply][0] = m;
                }
            }
            break;
        }
    }

    // Never store a partial result from an aborted iteration
    if (!stopped) {
        const Bound bound = (best <= alphaOrig) ? Bound::UPPER // failed low
                            : (best >= beta)    ? Bound::LOWER // failed high
                                                : Bound::EXACT;
        tt.store(g.board.zobristHash, depth, ply, best, bound, bestMove);
    }
    return best;
}

void Searcher::ageHistory() {
    for (auto &row : history) {
        for (auto &v : row)
            v /= 2;
    }
}

int64_t Searcher::msSince(Clock::time_point t) const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t).count();
}

void Searcher::fillStats(SearchResult &r) {
    r.nodes = nodes;
    r.qnodes = qnodes;
    r.cutoffs = cutoffs;
    r.firstCutoffs = firstCutoffs;
    r.seldepth = seldepth;
    r.ms = msSince(start);
}

bool Searcher::outOfTime() {
    if (stopped)
        return true;
    if (!limits.hardMs)
        return false;
    if ((nodes & 1023) != 0)
        return false; // infrequent checks
    if (msSince(start) >= *limits.hardMs)
        stopped = true;
    return stopped;
}

SearchResult Searcher::searchRoot(Game &g, int depth) {
    int scores[256];
    for (int i = 0; i < rootMoves.size(); ++i) {
        scores[i] = moveScore(g.board, rootMoves.moves[i], NO_MOVE, nullptr, nullptr);
        if (rootMoves.moves[i] == prevBest)
            scores[i] = INT_MAX; // marked as best will cut down alpha & beta
    }

    SearchResult r;
    r.move = rootMoves.moves[0];
    r.depth = depth;

    int alpha = -INF;
    MoveUndo undo;

    for (int i = 0; i < rootMoves.size(); ++i) {
        pickNext(rootMoves, scores, i);
        const Move m = rootMoves.moves[i];

        g.makeMove(m, undo);
        const int score = -negamax(g, depth - 1, 1, -INF, -alpha);
        g.undoMove(m, undo);

        if (stopped)
            break; // incomplete iteration, caller discards it

        if (i == 0 || score > alpha) {
            alpha = score;
            r.move = m;
            r.score = score;
        }
    }
    return r;
}

SearchResult Searcher::search(Game &g, const MoveList &root, SearchLimits lim) {
    nodes = 0;
    qnodes = 0;
    cutoffs = 0;
    firstCutoffs = 0;
    seldepth = 0;
    stopped = false;
    limits = lim;
    start = Clock::now();
    prevBest = NO_MOVE;
    rootMoves = root;
    for (auto &k : killers) {
        k[0] = NO_MOVE;
        k[1] = NO_MOVE;
    }

    SearchResult best;
    if (root.size() == 0)
        return best; // mated or stalemated
    best.move = rootMoves.moves[0];

    for (int d = 1; d <= limits.maxDepth; ++d) {
        const auto iterStart = Clock::now();
        const SearchResult r = searchRoot(g, d);

        if (stopped)
            break; // keep the last fully completed depth

        best = r;
        prevBest = r.move;

        if (limits.onIteration) {
            fillStats(best);
            limits.onIteration(best);
        }

        if (isMateScore(r.score))
            break; // more depth won't change a forced mate

        if (limits.softMs) {
            const int64_t iterMs = msSince(iterStart);
            if (msSince(start) + iterMs * 3 >= *limits.softMs)
                break;
        }
    }

    fillStats(best);
    return best;
}