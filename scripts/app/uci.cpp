#include "uci.hpp"
#include "game.hpp"
#include "movegen.hpp"
#include "notation.hpp"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <charconv>

namespace {

constexpr char ENGINE_NAME[] = "Cera";
constexpr char ENGINE_AUTHOR[] = "Isaac Honeyman";

bool debugInfo = true;

// UCI reports mate in MOVES, signed, not plies.
int mateIn(int score) {
    const int plies = MATE - std::abs(score);
    return (score > 0 ? 1 : -1) * ((plies + 1) / 2);
}

std::string human(uint64_t n) {
    char buf[32];
    if (n < 1000)         std::snprintf(buf, sizeof buf, "%llu", (unsigned long long)n);
    else if (n < 1000000) std::snprintf(buf, sizeof buf, "%.1fk", double(n) / 1e3);
    else                  std::snprintf(buf, sizeof buf, "%.2fM", double(n) / 1e6);
    return buf;
}

std::string pct(uint64_t part, uint64_t whole) {
    if (whole == 0) return "-";
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.1f%%", 100.0 * double(part) / double(whole));
    return buf;
}

std::string scoreText(int score) {
    char buf[32];
    if (isMateScore(score)) std::snprintf(buf, sizeof buf, "#%+d", mateIn(score));
    else                    std::snprintf(buf, sizeof buf, "%+.2f", double(score) / 100.0);
    return buf;
}

uint64_t npsOf(const SearchResult& r) { return r.nodes * 1000 / uint64_t(r.ms); }

void printInfo(const SearchResult& r, Move best) {
    std::cout << "info depth " << r.depth << " seldepth " << r.seldepth << " score ";
    if (isMateScore(r.score)) std::cout << "mate " << mateIn(r.score);
    else                      std::cout << "cp " << r.score;
    std::cout << " nodes " << r.nodes;
    if (r.ms > 0) std::cout << " nps " << npsOf(r);
    std::cout << " time " << r.ms << " pv " << toUCI(best) << std::endl;
}

void printDebug(const SearchResult& r, Move best, uint64_t prevNodes) {
    if (!debugInfo) return;
    if (prevNodes == 0)  // first row of this search
        std::cerr << "\n   d  sel      time      nodes       nps    ebf   qnode%    ord%"
                     "    score  pv\n";

    char ebf[16] = "-";
    if (prevNodes > 0) std::snprintf(ebf, sizeof ebf, "%.2f", double(r.nodes) / double(prevNodes));

    std::cerr << std::setw(4)  << r.depth
              << std::setw(5)  << r.seldepth
              << std::setw(10) << (std::to_string(r.ms) + "ms")
              << std::setw(11) << human(r.nodes)
              << std::setw(10) << (r.ms > 0 ? human(npsOf(r)) : std::string("-"))
              << std::setw(7)  << ebf
              << std::setw(9)  << pct(r.qnodes, r.nodes)
              << std::setw(8)  << pct(r.firstCutoffs, r.cutoffs)
              << std::setw(9)  << scoreText(r.score)
              << "  " << toUCI(best) << '\n';
}

std::vector<std::string> split(const std::string& line) {
    std::istringstream ss(line);
    std::vector<std::string> out;
    for (std::string tok; ss >> tok;) out.push_back(tok);
    return out;
}

void handlePosition(Game& g, const std::vector<std::string>& t) {
    size_t i = 1;
    if (i >= t.size()) return;

    if (t[i] == "startpos") {
        g.setFEN(START_FEN);
        ++i;
    } else if (t[i] == "fen") {
        ++i;
        std::string fen;
        for (; i < t.size() && t[i] != "moves"; ++i) {
            if (!fen.empty()) fen += ' ';
            fen += t[i];
        }
        if (fen.empty()) return;
        g.setFEN(fen);
    } else {
        return;  // malformed
    }

    if (i >= t.size() || t[i] != "moves") return;

    for (++i; i < t.size(); ++i) {
        MoveList legal;
        generateLegal(g.board, legal);
        const Move m = fromUCI(legal, t[i]);
        if (m == NO_MOVE) {
            std::cerr << "uci: rejected move '" << t[i] << "'\n";
            return;
        }
        g.playMove(m);
    }
}

constexpr int64_t MOVES_REMAINING = 40; // est moves remaining higher faster less thought
constexpr int64_t LAG_OVERHEAD_MS = 50;   // network + GUI round trip ests

int64_t number(const std::vector<std::string>& t, size_t i) {
    if (i >= t.size()) return 0;
    int64_t v = 0;
    std::from_chars(t[i].data(), t[i].data() + t[i].size(), v);
    return v;
}

SearchLimits parseGo(const std::vector<std::string>& t, PieceColour us) {
    int64_t wtime = 0, btime = 0, winc = 0, binc = 0, movetime = 0, movestogo = 0;
    int depth = 0;
    bool infinite = false;

    for (size_t i = 1; i < t.size(); ++i) {
        if      (t[i] == "wtime")     wtime     = number(t, ++i);
        else if (t[i] == "btime")     btime     = number(t, ++i);
        else if (t[i] == "winc")      winc      = number(t, ++i);
        else if (t[i] == "binc")      binc      = number(t, ++i);
        else if (t[i] == "movetime")  movetime  = number(t, ++i);
        else if (t[i] == "movestogo") movestogo = number(t, ++i);
        else if (t[i] == "depth")     depth     = int(number(t, ++i));
        else if (t[i] == "infinite")  infinite  = true;
    }

    SearchLimits lim;
    if (depth > 0) lim.maxDepth = depth;
    if (infinite) {
        if (depth == 0) lim.maxDepth = 10;
        return lim;                      // no clock, run to maxDepth
    }

    if (movetime > 0) {
        lim.softMs = lim.hardMs = std::max<int64_t>(1, movetime - LAG_OVERHEAD_MS);
        return lim;
    }

    const int64_t left = (us == WHITE) ? wtime : btime;
    const int64_t inc  = (us == WHITE) ? winc  : binc;
    if (left <= 0) return lim;                     // no clock info given

    const int64_t usable = std::max<int64_t>(1, left - LAG_OVERHEAD_MS);
    const int64_t base = (movestogo > 0) ? usable / movestogo
                                         : usable / MOVES_REMAINING + inc * 3 / 4;

    // Never commit more than half the remaining clock, however generous the
    lim.softMs = std::min(base, usable / 2);
    lim.hardMs = std::min(base * 3, usable / 2);
    return lim;
}

void handleGo(Game& g, Bot& bot, const std::vector<std::string>& t) {
    MoveList legal;
    generateLegal(g.board, legal);

    if (legal.size() == 0) {  // mated or stalemated
        std::cout << "bestmove 0000" << std::endl;
        return;
    }

    SearchLimits limits = parseGo(t, g.board.toMove);

    uint64_t prevNodes = 0;
    bool reported = false;
    limits.onIteration = [&](const SearchResult& r) {
        const Move m = (r.move == NO_MOVE) ? legal.moves[0] : r.move;
        printInfo(r, m);
        printDebug(r, m, prevNodes);
        prevNodes = r.nodes;
        reported = true;
    };

    const SearchResult r = bot.pick(g, legal, limits);
    const Move best = (r.move == NO_MOVE) ? legal.moves[0] : r.move;

    // A search aborted inside its first iteration -- or a bot that ignores the
    // callback -- reports nothing, so the GUI still gets one line.
    if (!reported) {
        printInfo(r, best);
        printDebug(r, best, prevNodes);
    }

    std::cout << "bestmove " << toUCI(best) << std::endl;
}

}  // namespace

void uciLoop(Bot& bot) {
    std::ios::sync_with_stdio(false);

    Game g;
    std::string line;

    while (std::getline(std::cin, line)) {
        const std::vector<std::string> t = split(line);
        if (t.empty()) continue;
        const std::string& cmd = t[0];

        if (cmd == "uci") {
            std::cout << "id name " << ENGINE_NAME << '\n'
                      << "id author " << ENGINE_AUTHOR << '\n'
                      << "uciok" << std::endl;
        } else if (cmd == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (cmd == "ucinewgame") {
            bot.newGame();
            g.setFEN(START_FEN);
        } else if (cmd == "position") {
            handlePosition(g, t);
        } else if (cmd == "go") {
            handleGo(g, bot, t);
        } else if (cmd == "debug") {
            if (t.size() > 1) debugInfo = (t[1] != "off");
        } else if (cmd == "stop" || cmd == "setoption") {
            // stop: nothing to interrupt while search is synchronous
            // setoption: no options declared yet
        } else if (cmd == "quit") {
            break;
        } else if (cmd == "d") {
            g.board.printBoard();  // stderr, so it can't corrupt the protocol
        }
    }
}