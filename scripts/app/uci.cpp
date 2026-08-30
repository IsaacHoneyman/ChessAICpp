#include "uci.hpp"
#include "game.hpp"
#include "movegen.hpp"
#include "notation.hpp"
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <charconv>

namespace {

constexpr char ENGINE_NAME[] = "Cera";
constexpr char ENGINE_AUTHOR[] = "Isaac Honeyman";

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

    const SearchLimits limits = parseGo(t, g.board.toMove);
    const SearchResult r = bot.pick(g, legal, limits);
    const Move best = (r.move == NO_MOVE) ? legal.moves[0] : r.move;

    if (isMateScore(r.score)) {
        // UCI reports mate in MOVES, signed, not plies.
        const int plies = MATE - std::abs(r.score);
        const int mateIn = (r.score > 0 ? 1 : -1) * ((plies + 1) / 2);
        std::cout << "info depth " << r.depth << " score mate " << mateIn
                  << " nodes " << r.nodes << " pv " << toUCI(best) << std::endl;
    } else {
        std::cout << "info depth " << r.depth << " score cp " << r.score
                  << " nodes " << r.nodes << " pv " << toUCI(best) << std::endl;
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
        } else if (cmd == "stop" || cmd == "setoption" || cmd == "debug") {
            // stop: nothing to interrupt while search is synchronous
            // setoption: no options declared yet
        } else if (cmd == "quit") {
            break;
        } else if (cmd == "d") {
            g.board.printBoard();  // stderr, so it can't corrupt the protocol
        }
    }
}